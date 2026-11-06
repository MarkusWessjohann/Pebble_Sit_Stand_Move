'use strict';

var config = require('./config.json');

var PIN_ID = '56fe7357-4f1c-4f59-8a93-f9e348a387f3-session-phase';
var STORAGE_KEY = 'sit_stand_move_timeline_snapshot';
var TIMELINE_URL = 'https://timeline-api.getpebble.com/v1/user/pins/';
var PHASE_NAMES = { 1: 'Sit', 2: 'Stand', 3: 'Move' };

function createBridge(options) {
  var pebble = options.pebble;
  var request = options.request;
  var token = options.token;
  var storage = options.storage;
  var schedule = options.schedule;
  var setTimeoutFn = options.setTimeout || setTimeout;
  var clearTimeoutFn = options.clearTimeout || clearTimeout;
  var latest = null;
  var latestGeneration = -1;
  var retryTimer = null;
  var requestInFlight = false;
  var reconcileAfterRequest = false;

  function integer(value) {
    return typeof value === 'number' && isFinite(value) && Math.floor(value) === value;
  }

  function validDeadline(value) {
    return integer(value) && value > 0 && value <= 2147483647;
  }

  function validSnapshot(snapshot) {
    if (!snapshot || !integer(snapshot.generation) || snapshot.generation < 0) return false;
    if (snapshot.lifecycle === 0) return snapshot.phase === 0;
    if (snapshot.lifecycle === 1) {
      return !!PHASE_NAMES[snapshot.phase] && validDeadline(snapshot.deadlineUtc);
    }
    if (snapshot.lifecycle === 2) {
      return !!PHASE_NAMES[snapshot.phase] && integer(snapshot.remainingSeconds) &&
        snapshot.remainingSeconds > 0;
    }
    if (snapshot.lifecycle === 3) {
      return !!PHASE_NAMES[snapshot.phase] && validDeadline(snapshot.pauseDeadlineUtc) &&
        integer(snapshot.remainingSeconds) && snapshot.remainingSeconds > 0;
    }
    return false;
  }

  function persistSnapshot() {
    storage.setItem(STORAGE_KEY, JSON.stringify({ generation: latestGeneration, snapshot: latest }));
  }

  function send(method, pin) {
    if (!token) return;
    if (requestInFlight) {
      reconcileAfterRequest = true;
      return;
    }
    requestInFlight = true;
    var completed = false;
    var timeout = setTimeoutFn(function () { done(false, 0); }, 30000);
    function done(ok, status) {
      if (completed) return;
      completed = true;
      clearTimeoutFn(timeout);
      requestInFlight = false;
      if (ok || (method === 'DELETE' && status === 404)) {
        if (retryTimer !== null) {
          clearTimeout(retryTimer);
          retryTimer = null;
        }
      } else if (retryTimer === null) {
        retryTimer = schedule(function () {
          retryTimer = null;
          reconcile();
        }, 30000);
      }
      if (reconcileAfterRequest) {
        reconcileAfterRequest = false;
        reconcile();
      }
    }
    try {
      request(method, TIMELINE_URL + encodeURIComponent(PIN_ID), token, pin, done);
    } catch (error) {
      done(false, 0);
    }
  }

  function reconcile() {
    if (!latest || !token) return;
    if (latest.lifecycle !== 1 && latest.lifecycle !== 3) {
      send('DELETE', null);
      return;
    }
    var phase = PHASE_NAMES[latest.phase];
    var phaseEnd = latest.lifecycle === 3 ? latest.pauseDeadlineUtc + latest.remainingSeconds :
                   latest.deadlineUtc;
    if (!phase || !validDeadline(phaseEnd)) return;
    send('PUT', {
      id: PIN_ID,
      time: new Date(phaseEnd * 1000).toISOString(),
      layout: {
        type: 'genericPin',
        title: phase + ' ends at ' + new Date(phaseEnd * 1000).toLocaleTimeString([], {
          hour: 'numeric', minute: '2-digit'
        }),
        body: 'Your ' + phase.toLowerCase() + ' phase is expected to end.'
      }
    });
  }

  function onMessage(event) {
    var data = event && event.payload;
    if (!data || data[0] !== 1 || !integer(data[4]) || data[4] < latestGeneration) return;
    var snapshot = {
      lifecycle: data[2],
      phase: data[3],
      generation: data[4],
      deadlineUtc: data[5],
      remainingSeconds: data[6],
      pauseDeadlineUtc: data[7]
    };
    if (!validSnapshot(snapshot)) return;
    if (data[4] === latestGeneration && latest) return;
    latestGeneration = data[4];
    latest = snapshot;
    persistSnapshot();
    reconcile();
  }

  function restore() {
    var stored;
    try {
      stored = JSON.parse(storage.getItem(STORAGE_KEY));
    } catch (error) {
      return;
    }
    if (stored && integer(stored.generation) && stored.snapshot &&
        validSnapshot(stored.snapshot) && stored.generation === stored.snapshot.generation) {
      latestGeneration = stored.generation;
      latest = stored.snapshot;
    }
  }

  pebble.addEventListener('appmessage', onMessage);
  pebble.addEventListener('ready', function () {
    restore();
    reconcile();
    pebble.sendAppMessage({ 1: 1 }, function () {}, function () {});
  });
  return { onMessage: onMessage, reconcile: reconcile, getLatest: function () { return latest; } };
}

if (typeof Pebble !== 'undefined') {
  createBridge({
    pebble: Pebble,
    token: config.timelineApiToken,
    storage: localStorage,
    schedule: setTimeout,
    setTimeout: setTimeout,
    clearTimeout: clearTimeout,
    request: function (method, url, token, body, done) {
      var xhr = new XMLHttpRequest();
      xhr.open(method, url);
      xhr.setRequestHeader('X-User-Token', token);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onload = function () { done(xhr.status >= 200 && xhr.status < 300, xhr.status); };
      xhr.onerror = function () { done(false, xhr.status); };
      xhr.ontimeout = function () { done(false, xhr.status); };
      xhr.timeout = 30000;
      xhr.send(body ? JSON.stringify(body) : null);
    }
  });
}

if (typeof module !== 'undefined') module.exports = { createBridge: createBridge };
