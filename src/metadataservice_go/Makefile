-include env
include env

UNAME := $(shell uname)
GO_PATH := $(shell which go)

ifeq ($(UNAME), Linux)
MDS_BUILD_PATH = ${MDS_BUILD_PATH_LINUX}
SOURCE = .
ifeq ($(GO_PATH),)
	GO_PATH = ${GO_PATH_LINUX}
endif
endif

ifeq ($(UNAME), Darwin)
MDS_BUILD_PATH = ${MDS_BUILD_PATH_DARWIN}
SOURCE = source
endif

.PHONY: help
## help: prints this help message
help:
	@echo "Usage: \n"
	@sed -n 's/^##//p' ${MAKEFILE_LIST} | column -t -s ':' |  sed -e 's/^/ /'

.PHONY: tidy
## tidy: tidy up go modules
tidy:
	@${GO_PATH} mod tidy

.PHONY: vendor
## vendor: vendoring the modules
vendor:
	@${GO_PATH} mod vendor -v

.PHONY: test
## test: running the tests
test:
	@echo "Running tests with persistent storage enabled"
	@GEDS_PERSISTENT_STORAGE_MDS_ENABLED=true ${GO_PATH} test ./tests -v
	@echo "Running tests with in-memory storage enabled"
	@GEDS_PERSISTENT_STORAGE_MDS_ENABLED=false ${GO_PATH} test ./tests -v

.PHONY: clean
## clean: clean build path for Darwin
clean:
	@echo "Cleaning"
	@${GO_PATH} clean
	@rm -rf ${MDS_BUILD_PATH}

.PHONY: clean-all
## clean-all: clean everything
clean-all:
	@echo "Cleaning"
	@${GO_PATH} clean
	@rm -rf ${MDS_BUILD_PATH_LINUX}
	@rm -rf ${MDS_BUILD_PATH_DARWIN}

.PHONY: build
## build: build the MDS
build: clean build-mds

.PHONY: build-mds
## build-mds: build the MDS component (OS-dependent)
build-mds:
	@echo "Building MDS ..."
	@${GO_PATH} build -o ${MDS_BUILD_PATH}${MDS_BINARY} ./cmd/mds/*.go

## run-mds: run the MDS component
run-mds:
	@echo "Running MDS ..."
	@${GO_PATH} run ./cmd/mds

## run-mock-client: run the MockClient
run-mock-client:
	@echo "Running Mock-Client ..."
	@${GO_PATH} run ./cmd/mockclient

.PHONY: protos
## protos: generate the protos
protos:
	@echo "Generating the Protos ..."
	@rm -rf ./protos;
	@mkdir ./protos
	@protoc --experimental_allow_proto3_optional -I ../protos ../protos/*.proto  --go_out=./protos
	@protoc --experimental_allow_proto3_optional -I ../protos ../protos/*.proto  --go-grpc_out=require_unimplemented_servers=false:./protos


.PHONY: build-docker-images
## build-docker-images: build docker images
build-docker-images:
	@echo "Building Docker images ..."
	@docker build --force-rm -t ${DOCKER_REGISTRY}/geds-mds-go -f ./deployment/docker/images/geds-mds-go.dockerfile .
	@docker tag ${DOCKER_REGISTRY}/geds-mds-go ${DOCKER_REGISTRY}/geds-mds-go:${DOCKER_TAG_ENV}

.PHONY: push-docker-images
## push-docker-images: push docker images
push-docker-images:
	@echo "Pushing Docker images ..."
	@docker push ${DOCKER_REGISTRY}/geds-mds-go:${DOCKER_TAG_ENV}
