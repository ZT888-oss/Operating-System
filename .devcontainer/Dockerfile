FROM debian:trixie-20241223

ARG USERNAME=codespace
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y \
        build-essential \
        clang \
        clangd \
        cmake \
        e2fsprogs \
        fish \
        gdb \
        git \
        lldb \
        locales \
        meson \
        nano \
        strace \
        sudo \
        valgrind \
        vim \
        wget \
        zsh \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen

USER $USERNAME

RUN git config --global pull.rebase false \
    && echo 'settings set target.disable-aslr false' >> /home/$USERNAME/.lldbinit
RUN sh -c "$(wget https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh -O -)" "" --unattended \
    && sed -i 's/ZSH_THEME="robbyrussell"/ZSH_THEME="lukerandall"/g' /home/$USERNAME/.zshrc \
    && echo 'disable test\nexport PATH=/workspace/.bin:$PATH' >> /home/$USERNAME/.zshrc

CMD ["zsh"]
