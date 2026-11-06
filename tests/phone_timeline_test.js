'use strict';

var assert = require('node:assert/strict');
var fs = require('node:fs');
var path = require('node:path');
var test = require('node:test');
var createBridge = require('../src/pkjs/index.js').createBridge;

function harness() {
  var listeners = {};
  var requests = [];
  var storage = {};
  var scheduled = [];
  var requestTimeouts = [];
  var sentMessages = [];
  var bridge = createBridge({
    pebble: {
      addEventListener: function (name, callback) { listeners[name] = callback; },
      sendAppMessage: function (payload, success) { sentMessages.push(payload); success(); }
    },
    request: function (method, url, token, body, done) {
      requests.push({ method: method, url: url, token: token, body: body, done: done });
    },
    token: 'test-token',
    storage: {
      getItem: function (key) { return storage[key] || null; },
      setItem: function (key, value) { storage[key] = value; }
    },
    schedule: function (callback, delay) { scheduled.push({ callback: callback, delay: delay }); return scheduled.length; },
    setTimeout: function (callback, delay) { requestTimeouts.push({ callback: callback, delay: delay }); return requestTimeouts.length; },
    clearTimeout: function (id) { if (requestTimeouts[id - 1]) requestTimeouts[id - 1].cleared = true; }
  });
  return { listeners: listeners, requests: requests, storage: storage, scheduled: scheduled,
    requestTimeouts: requestTimeouts, sentMessages: sentMessages, bridge: bridge };
}

function snapshot(lifecycle, phase, generation, deadline, remaining, pauseDeadline) {
  return { payload: { 0: 1, 2: lifecycle, 3: phase, 4: generation, 5: deadline,
    6: remaining || 0, 7: pauseDeadline || 0 } };
}

test('running snapshot creates stable informational pin at UTC deadline', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 4, 1800002700));
  assert.equal(h.requests.length, 1);
  assert.equal(h.requests[0].method, 'PUT');
  assert.match(h.requests[0].url, /56fe7357-4f1c-4f59-8a93-f9e348a387f3-session-phase$/);
  assert.equal(h.requests[0].body.id, '56fe7357-4f1c-4f59-8a93-f9e348a387f3-session-phase');
  assert.equal(h.requests[0].body.time, new Date(1800002700 * 1000).toISOString());
  assert.equal(h.requests[0].body.layout.title, 'Sit ends at ' +
    new Date(1800002700 * 1000).toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' }));
  assert.equal(h.requests[0].body.reminders, undefined);
});

test('timed pause publishes the predicted phase end', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(3, 2, 5, 0, 420, 1800000300));
  assert.equal(h.requests[0].body.time, new Date(1800000720 * 1000).toISOString());
  assert.match(h.requests[0].body.layout.title, /^Stand ends at /);
});

test('newer snapshot wins, stale generation cannot roll back, and pin id stays stable', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 10, 1800002700));
  h.requests[0].done(true, 200);
  h.listeners.appmessage(snapshot(1, 2, 11, 1800003300));
  h.requests[1].done(true, 200);
  h.listeners.appmessage(snapshot(1, 1, 10, 1800002700));
  assert.equal(h.requests.length, 2);
  assert.equal(h.requests[1].body.id, h.requests[0].body.id);
  assert.match(h.requests[1].body.layout.title, /^Stand ends at /);
});

test('inactive and manual pause delete the pin idempotently', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(0, 0, 1, 0));
  assert.equal(h.requests[0].method, 'DELETE');
  h.requests[0].done(false, 404);
  h.listeners.appmessage(snapshot(2, 1, 2, 0));
  assert.equal(h.requests.length, 1);
});

test('manual pause deletes an existing pin and retry reconciles latest deletion', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 1, 1800002700));
  h.requests[0].done(true, 200);
  h.listeners.appmessage(snapshot(2, 1, 2, 0, 200));
  assert.equal(h.requests[1].method, 'DELETE');
  h.requests[1].done(false, 503);
  h.scheduled[0].callback();
  assert.equal(h.requests[2].method, 'DELETE');
});

test('failed publish retries latest snapshot rather than stale deadline', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 1, 1800002700));
  h.requests[0].done(false, 503);
  assert.equal(h.scheduled.length, 1);
  assert.equal(h.scheduled[0].delay, 30000);
  h.listeners.appmessage(snapshot(1, 3, 2, 1800003000));
  h.scheduled[0].callback();
  assert.equal(h.requests[1].method, 'PUT');
  assert.equal(h.requests[1].body.time, new Date(1800003000 * 1000).toISOString());
});

test('ready restores saved snapshot and reconciles it', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 8, 1800002700));
  h.requests[0].done(true, 200);
  var restored = harness();
  Object.assign(restored.storage, h.storage);
  restored.listeners.ready();
  assert.equal(restored.requests.length, 1);
  assert.equal(restored.requests[0].body.time, new Date(1800002700 * 1000).toISOString());
});

test('rejects malformed lifecycle and deadline snapshots without replacing current state', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 4, 1800002700));
  h.requests[0].done(true, 200);
  h.listeners.appmessage(snapshot(99, 1, 5, 1800003300));
  h.listeners.appmessage(snapshot(1, 99, 6, 1800003300));
  h.listeners.appmessage(snapshot(1, 1, 7, 0));
  assert.equal(h.requests.length, 1);
  assert.equal(h.bridge.getLatest().generation, 4);
});

test('stalled requests time out and reconcile the latest snapshot', function () {
  var h = harness();
  h.listeners.appmessage(snapshot(1, 1, 1, 1800002700));
  h.listeners.appmessage(snapshot(1, 2, 2, 1800003300));
  assert.equal(h.requests.length, 1);
  h.requestTimeouts[0].callback();
  assert.equal(h.scheduled.length, 1);
  h.scheduled[0].callback();
  assert.equal(h.requests.length, 2);
  assert.match(h.requests[1].body.layout.title, /^Stand ends at /);
});

test('phone bridge startup requests the latest snapshot from the watch', function () {
  var h = harness();
  h.listeners.ready();
  assert.deepEqual(h.sentMessages, [{ 1: 1 }]);
});

test('manifest, C sender, and JS parser share the AppMessage field contract', function () {
  var root = path.resolve(__dirname, '..');
  var keys = require(path.join(root, 'package.json')).pebble.messageKeys;
  var watch = fs.readFileSync(path.join(root, 'src/c/sit_stand_move.c'), 'utf8');
  var phone = fs.readFileSync(path.join(root, 'src/pkjs/index.js'), 'utf8');
  assert.deepEqual(keys, ['snapshot_version', 'refresh_snapshot', 'lifecycle', 'phase',
    'generation', 'deadline_utc', 'remaining_seconds', 'pause_deadline_utc']);
  keys.forEach(function (key, index) {
    assert.match(watch, new RegExp('MESSAGE_KEY_' + key.toUpperCase() + '\\s*=\\s*' + index));
  });
  assert.match(phone, /lifecycle: data\[2\]/);
  assert.match(phone, /phase: data\[3\]/);
  assert.match(phone, /generation: data\[4\]/);
  assert.match(phone, /deadlineUtc: data\[5\]/);
  assert.match(phone, /remainingSeconds: data\[6\]/);
  assert.match(phone, /pauseDeadlineUtc: data\[7\]/);
});
