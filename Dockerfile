# Simple Dockerfile for C++ on Render
FROM ubuntu:22.04

# Install minimal dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Simple compilation
RUN g++ -std=c++17 -o fitness_quest main.cpp -lssl -lcrypto -lpthread

# Render uses PORT=10000
ENV PORT=10000
EXPOSE $PORT

# Run the app
CMD ["./fitness_quest"]
