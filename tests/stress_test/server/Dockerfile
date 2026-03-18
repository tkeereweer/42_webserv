FROM debian:12
WORKDIR /webserv

RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    make \
    git \
    python3 \
    php-cgi \
    vim \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

EXPOSE 9090

RUN mkdir ressources

COPY startup.sh .
RUN chmod +x startup.sh

ENTRYPOINT [ "./startup.sh" ]
