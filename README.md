# Arduino Client for MQTT

This library is a fork of [pubsubclient from mkjnc](https://github.com/mknjc/pubsubclient)
which again is a fork of [pubsubclient from knolleary](https://github.com/knolleary/pubsubclient).

Both patches together improve streaming support for MQTT. For a full README see to
the original library.

The fork from mjnc adds 32-bit support for message sizes, see [pr](https://github.com/knolleary/pubsubclient/pull/170)

This fork adds much more efficient reading in blocks and provides callbacks for received buffers.

## License
This code is released under the MIT License.
