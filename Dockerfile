FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ make && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY Makefile ./
COPY include include
COPY src src
COPY tests tests
RUN make all test

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /app/bin/playlist-builder /app/bin/playlist-builder
COPY public public
ENV PORT=8080
EXPOSE 8080
CMD ["./bin/playlist-builder"]
