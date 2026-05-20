FROM fedora:44 AS builder

RUN dnf install glibc-devel libstdc++-devel git gh cmake make gcc gcc-c++ gettext-devel -y

RUN mkdir /app
WORKDIR /app

RUN git clone --branch docker https://github.com/Azafanatic/dyskont-sim.git dyskont
RUN chmod +x /app/dyskont/build.sh

WORKDIR /app/dyskont
RUN /app/dyskont/build.sh

FROM fedora:44 AS production

RUN mkdir -p /app/dyskont
WORKDIR /app/dyskont

COPY --from=builder /app/dyskont/build/* /app/dyskont/

ENTRYPOINT ["/app/dyskont/sim", "3600", "180", "0"]
