FROM debian:12
WORKDIR /data

RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    make \
    git \
    python3 \
    php-cgi \
    vim \
    siege \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]