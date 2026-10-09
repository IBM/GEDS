FROM golang:1.26-alpine3.24 AS builder

RUN apk add --no-cache \
	bash \
	make

WORKDIR /gedsmds
ADD . /gedsmds/
COPY ./env.secret /gedsmds/env

RUN make build-mds

FROM alpine:3.24

ENV GEDSMDS_SERVER_PORT=50004

WORKDIR /gedsmds
COPY --from=builder /gedsmds/build/gedsmds_linux/gedsmds .
COPY --from=builder /gedsmds/configs/app.env.secret ./app.env

EXPOSE $GEDSMDS_SERVER_PORT
CMD ["./gedsmds"]
