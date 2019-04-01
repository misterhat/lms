# LMS - Lightweight Music Server

LMS is a self-hosted music streaming software: access your music files from anywhere using a web interface!

A [demo](http://lms.demo.poupon.io) instance is available, with the following limitations:
- Settings cannot be saved
- No persistent playqueue

## Main features
LMS provides several ways to help you find the music you like:
- Tag-based filters (ex: "Metal" and "Aggressive", "Electronic" and "Relaxed", ...)
- Recommendations for similar artists and albums
- Radio mode
- Searches in album, artist and track names
- Most played/Recently added music

Conventional features include:
- Responsive design
- Persistent play queue across sessions
- User management
- MusicBrainzID support to handle duplicated artist and release names
- Audio transcode for maximum interoperability and low bandwith requirements
- Multi-value tags
- Custom tag support (ex: "mood", "genre", "albummood", "albumgrouping", ...)

The recommendation engine makes use of [Self-Organizing Maps](https://en.wikipedia.org/wiki/Self-organizing_map).</br>
Please note this engine:
- may require some significant computation time on very large datasets
- makes use of computed data available on [AcousticBrainz](https://acousticbrainz.org/). Therefore your music must contain the [MusicBrainz Identifier](https://musicbrainz.org/doc/MusicBrainz_Identifier) for the recommendation engine to work properly

## Installation
Here are the required packages to build on Debian Stretch:
```sh
apt-get install g++ autoconf automake libboost-filesystem-dev libboost-system-dev libavcodec-dev libavutil-dev libavformat-dev libav-tools libmagick++-dev libpstreams-dev libconfig++-dev libpstreams-dev ffmpeg libtag1-dev
```

You also need wt4, that is not packaged yet on Debian. See [installation instructions](https://www.webtoolkit.eu/wt/doc/reference/html/InstallationUnix.html).

```sh
git clone https://github.com/epoupon/lms.git lms
cd lms
autoreconf -vfi
mkdir build
cd build
../configure --prefix=/usr --sysconfdir=/etc
```
configure will report any missing library.

```sh
make
```

```sh
make install
```
This command requires root privileges.

## Configuration
LMS uses a configuration file, installed in '/etc/lms.conf'. It is recommended to edit this file and change the relevant settings (working directory, listen port, etc.)

All other settings are set using the web interface.

It is highly recommended to run LMS as a non root user. Therefore make sure the user has write permissions on the working directory.

## Reverse proxy settings
You have to set the 'behind-reverse-proxy' option to 'true' in the configuration file.

Here is an example to make LMS properly work on myserver.org using nginx.
```
server {
    listen 80;

    server_name myserver.org;

    access_log            /var/log/nginx/myserver.access.log;

    proxy_request_buffering off;
    proxy_buffering off;
    proxy_buffer_size 4k;

    location / {

      proxy_set_header        Host $host;
      proxy_set_header        X-Real-IP $remote_addr;
      proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
      proxy_set_header        X-Forwarded-Proto $scheme;

      proxy_pass          http://localhost:5091;
      proxy_read_timeout  120;
    }
}
```

## Running
```sh
lms [config_file]
```
Logs are output in the working directory, in the file 'lms.log'.

To connect to LMS, just open your favorite browser and go to http://localhost:5082

## Credits
- Wt (http://www.webtoolkit.eu/)
- bootstrap3 (http://getbootstrap.com/)
- ffmpeg project (https://ffmpeg.org/)
- Magick++ (http://www.imagemagick.org/Magick++/)
- MetaBrainz (https://metabrainz.org/)
- Bootstrap Notify: https://github.com/mouse0270/bootstrap-notify