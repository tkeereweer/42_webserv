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

EXPOSE 9090

RUN ls

# RUN chmod 0744 flood_test.sh && chmod 0744 random_urls_get_test_test.sh && chmod 0744 simple_test.sh

CMD ["/bin/bash"]