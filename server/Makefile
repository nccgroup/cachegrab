PROJ=cachegrab_server
SHIM=libcachegrab_shim.so
TMPDIR=/data/local/tmp

$(PROJ):
	ndk-build

deploy: $(PROJ)
	adb push libs/arm64-v8a/$(PROJ) $(TMPDIR)/$(PROJ)
	adb push libs/arm64-v8a/$(SHIM) $(TMPDIR)/libQSEEComAPI.so

run: deploy kill
	adb shell su -c '$(TMPDIR)/cachegrab_server'
	adb forward tcp:8000 tcp:8000

kill:
	./kill_server.sh
	adb forward --remove tcp:8000 || true

clean:
	ndk-build clean
	rm -rf *~ libs obj
