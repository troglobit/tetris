FROM alpine:3.5

RUN apk add --no-cache gcc musl-dev make; mkdir /var/games

# Install from GIT
WORKDIR .
ADD . /tetris
RUN cd tetris/; DESTDIR=/usr make all install

# Alternatively, install from released tarball
#RUN wget http://ftp.troglobit.com/tetris/tetris-1.2.1.tar.bz2; \
#    tar xfj tetris-1.2.1.tar.bz2; \
#    cd tetris-1.2.1/;		  \
#    DESTDIR=/usr make all install

CMD tetris
