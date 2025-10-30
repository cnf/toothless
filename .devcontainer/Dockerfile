ARG DOCKER_TAG=latest
FROM espressif/idf:${DOCKER_TAG}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN apt-get update -y && apt-get install udev -y
RUN apt-get install -y git wget flex bison gperf python3 \
        python3-pip python3-venv cmake ninja-build ccache \
        libffi-dev libssl-dev dfu-util libusb-1.0-0 \
        sudo linux-tools-virtual hwdata

# For mac, to have usbip
RUN rm /usr/local/bin/usbip ; ln -s $(ls /usr/lib/linux-tools/*/usbip) /usr/local/bin/usbip
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

RUN su -c 'echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc' ubuntu
RUN chown -R ubuntu:users /opt/esp
RUN usermod -a -G dialout ubuntu
RUN usermod -a -G plugdev ubuntu


ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]

CMD ["/bin/bash", "-c"]