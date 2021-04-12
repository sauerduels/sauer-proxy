# Dockerfile for creating a statically-linked Rust application using docker's
# multi-stage build feature. This also leverages the docker build cache to avoid
# re-downloading dependencies if they have not changed.
FROM rust:1.35.0 AS build
WORKDIR /usr/src

# Download the target for static linking.
RUN rustup target add x86_64-unknown-linux-musl
RUN rustup component add rustfmt

# Install custom dependency libclang-dev
RUN apt-get update \
    && apt-get install -y clang libclang-dev cmake \
    && ln -s /usr/bin/gcc /usr/bin/musl-gcc \
    && ln -s /usr/bin/g++ /usr/bin/musl-g++

# Create a dummy project and build the app's dependencies.
# If the Cargo.toml or Cargo.lock files have not changed,
# we can use the docker build cache and skip these (typically slow) steps.
RUN USER=root cargo new sauer-proxy
WORKDIR /usr/src/sauer-proxy
COPY Cargo.toml Cargo.lock ./
RUN cargo build --release

# Copy the source and build the application.
COPY src ./src
RUN cargo install --target x86_64-unknown-linux-musl --path .

# Copy the statically-linked binary into a scratch container.
FROM scratch
COPY --from=build /usr/local/cargo/bin/sauer-proxy .
USER 1000
ENTRYPOINT ["./sauer-proxy"]

EXPOSE 28785 28786
