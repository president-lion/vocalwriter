#!/bin/bash

cd build
mkdir armuniv-14.1.0

lipo -create armv6sim-14.1.0/dectalk/libtts.a arm64-14.1.0/dectalk/libtts.a armv7-14.1.0/dectalk/libtts.a armv7sim-14.1.0/dectalk/libtts.a -output armuniv-14.1.0/libtts.a
lipo -create armv6sim-14.1.0/dectalk/libtts_hb.a arm64-14.1.0/dectalk/libtts_hb.a armv7-14.1.0/dectalk/libtts_hb.a armv7sim-14.1.0/dectalk/libtts_hb.a -output armuniv-14.1.0/libtts_hb.a
lipo -create armv6sim-14.1.0/dectalk/libtts_ch_stub.a arm64-14.1.0/dectalk/libtts_ch_stub.a armv7-14.1.0/dectalk/libtts_ch_stub.a armv7sim-14.1.0/dectalk/libtts_ch_stub.a -output armuniv-14.1.0/libtts_ch_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_fr_stub.a arm64-14.1.0/dectalk/libtts_fr_stub.a armv7-14.1.0/dectalk/libtts_fr_stub.a armv7sim-14.1.0/dectalk/libtts_fr_stub.a -output armuniv-14.1.0/libtts_fr_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_gr_stub.a arm64-14.1.0/dectalk/libtts_gr_stub.a armv7-14.1.0/dectalk/libtts_gr_stub.a armv7sim-14.1.0/dectalk/libtts_gr_stub.a -output armuniv-14.1.0/libtts_gr_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_it_stub.a arm64-14.1.0/dectalk/libtts_it_stub.a armv7-14.1.0/dectalk/libtts_it_stub.a armv7sim-14.1.0/dectalk/libtts_it_stub.a -output armuniv-14.1.0/libtts_it_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_jp_stub.a arm64-14.1.0/dectalk/libtts_jp_stub.a armv7-14.1.0/dectalk/libtts_jp_stub.a armv7sim-14.1.0/dectalk/libtts_jp_stub.a -output armuniv-14.1.0/libtts_jp_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_kr_stub.a arm64-14.1.0/dectalk/libtts_kr_stub.a armv7-14.1.0/dectalk/libtts_kr_stub.a armv7sim-14.1.0/dectalk/libtts_kr_stub.a -output armuniv-14.1.0/libtts_kr_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_la_stub.a arm64-14.1.0/dectalk/libtts_la_stub.a armv7-14.1.0/dectalk/libtts_la_stub.a armv7sim-14.1.0/dectalk/libtts_la_stub.a -output armuniv-14.1.0/libtts_la_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_sp_stub.a arm64-14.1.0/dectalk/libtts_sp_stub.a armv7-14.1.0/dectalk/libtts_sp_stub.a armv7sim-14.1.0/dectalk/libtts_sp_stub.a -output armuniv-14.1.0/libtts_sp_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_uk_stub.a arm64-14.1.0/dectalk/libtts_uk_stub.a armv7-14.1.0/dectalk/libtts_uk_stub.a armv7sim-14.1.0/dectalk/libtts_uk_stub.a -output armuniv-14.1.0/libtts_uk_stub.a
lipo -create armv6sim-14.1.0/dectalk/libtts_us_stub.a arm64-14.1.0/dectalk/libtts_us_stub.a armv7-14.1.0/dectalk/libtts_us_stub.a armv7sim-14.1.0/dectalk/libtts_us_stub.a -output armuniv-14.1.0/libtts_us_stub.a


