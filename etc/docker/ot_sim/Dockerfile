FROM alpine:3.6 as openthread-dev
LABEL maintainer="Marcin K Szczodrak"

RUN apk add -U autoconf automake ca-certificates flex git g++ libtool linux-headers make

# openthread
RUN git clone --recursive https://github.com/openthread/openthread.git && \
    cd /openthread && \
    ./bootstrap && \
    make -f examples/Makefile-posix


FROM alpine:3.6
LABEL maintainer="Marcin K Szczodrak"

RUN apk add --no-cache libstdc++
COPY --from=openthread-dev /openthread/output/x86_64-unknown-linux-gnu/bin/ot-cli-ftd /bin/ot-cli-ftd
COPY --from=openthread-dev /openthread/output/x86_64-unknown-linux-gnu/bin/ot-cli-mtd /bin/ot-cli-mtd
RUN ln -s /bin/ot-cli-ftd /bin/node
