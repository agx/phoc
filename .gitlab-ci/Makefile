VERSION=$(shell date --iso)

all:
	./run-docker.sh build  --base debian --version 0.0.$(VERSION)

push:
	./run-docker.sh push  --base debian --version 0.0.$(VERSION)
