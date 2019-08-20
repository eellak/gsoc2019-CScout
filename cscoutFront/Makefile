INSTALL_PREFIX?=/usr/local

.PHONY: build/Front_End npm/serve

build/Front_End: npm/serve
	npm install
	npm run build

build:
	npm run build

npm/serve:	
	npm install -g serve

install:
	install -d "$(INSTALL_PREFIX)/cscoutFront/static/css"
	install -d "$(INSTALL_PREFIX)/cscoutFront/static/js"
	install -d "$(INSTALL_PREFIX)/cscoutFront/static/media"
	cd ./build && find ./ -type f -exec install -D {} "$(INSTALL_PREFIX)/cscoutFront/{}" \; 

uninstall:
	rm -rf "$(INSTALL_PREFIX)/cscoutFront"
