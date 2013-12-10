_                       = require('underscore');
assert                  = require('assert');
WebSocketHelper         = require('./WebSocketHelper');

function wshPipe(msg) {
  var msgParts = WebSocketHelper.stringify(msg);
  if (0) console.log(msgParts.json, msgParts.binaries);
  var msg2 = WebSocketHelper.parse(msgParts.json, msgParts.binaries);
  return msg2;
}

describe('WebSocketHelper', function() {
  it('should handle basic objects', function() {
    var msg2 = wshPipe({foo: 1, bar: 'buz'});
    assert.ok(msg2.foo === 1);
    assert.ok(msg2.bar === 'buz');
  });

  it('should handle ArrayBuffers', function() {
    var msg2 = wshPipe({foo: 1, bar: new ArrayBuffer(32)});
    assert.ok(msg2.foo === 1);
    assert.ok(msg2.bar.constructor === ArrayBuffer);
    assert.ok(msg2.bar.byteLength === 32);
  });

  it('should handle typed arrays', function() {
    _.each([Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array], function(t) {
      var msg2 = wshPipe({foo: 1, bar: new t(3)});
      assert.ok(msg2.foo === 1);
      assert.ok(msg2.bar.constructor === t);
      assert.ok(msg2.bar.length === 3);
    });
  });
});


describe('RpcPendingQueue', function() {
  it('should be efficient', function() {
    var iters = 50000;
    var outstanding = 10;
    var localq = [];

    var pending = new WebSocketHelper.RpcPendingQueue();
    for (var i=0; i<iters; i++) {
      var reqId = pending.getNewId();
      localq.push(reqId);
      pending.add(reqId, 'foo' + reqId + 'bar');

      if (localq.length >= outstanding) {
        var rspId = localq.shift();
        var rspFunc = pending.get(rspId);
        assert.strictEqual(rspFunc, 'foo' + rspId + 'bar');
      }
    }
    assert.equal(pending.pending.length, outstanding);
  });
});
