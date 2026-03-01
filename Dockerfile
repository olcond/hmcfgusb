# HMCFGUSB
# http://git.zerfleddert.de/cgi-bin/gitweb.cgi/hmcfgusb
FROM alpine:3.23 AS builder

COPY . /app/hmcfgusb
WORKDIR /app/hmcfgusb

RUN apk add --no-cache \
        build-base \
        clang \
        libusb-dev \
 && clang --version && make

FROM alpine:3.23

RUN apk add --no-cache \
        libusb \
        ca-certificates

COPY --from=builder /app/hmcfgusb/hmland \
                    /app/hmcfgusb/hmsniff \
                    /app/hmcfgusb/flash-hmcfgusb \
                    /app/hmcfgusb/flash-hmmoduart \
                    /app/hmcfgusb/flash-ota \
                    /app/hmcfgusb/

EXPOSE 1234

CMD ["/app/hmcfgusb/hmland", "-v", "-p", "1234", "-I"]
