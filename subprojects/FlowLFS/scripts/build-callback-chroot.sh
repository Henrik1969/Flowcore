#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
log_dir=/var/log/flowlfs/callback
marker="$log_dir/openssh.done"
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/build.log") 2>&1

if test -e "$marker"; then
  echo 'SKIP completed callback build'
  exit 0
fi

echo "===== START callback-openssh $(date --utc +'%Y-%m-%dT%H:%M:%SZ') ====="
cd /sources
printf '%s  %s\n' a95119f402dfa0166c9dd1237239085c openssh-10.5p1.tar.gz | md5sum --check
rm -rf openssh-10.5p1
tar -xf openssh-10.5p1.tar.gz
cd openssh-10.5p1

install -v -g sys -m700 -d /var/lib/sshd
getent group sshd >/dev/null || groupadd -g 50 sshd
id sshd >/dev/null 2>&1 || useradd -c 'sshd PrivSep' -d /var/lib/sshd \
  -g sshd -s /bin/false -u 50 sshd

./configure --prefix=/usr                            \
            --sysconfdir=/etc/ssh                    \
            --with-privsep-path=/var/lib/sshd        \
            --with-default-path=/usr/bin             \
            --with-superuser-path=/usr/sbin:/usr/bin \
            --with-pid-dir=/run
make
make -j1 tests
make install
install -v -m755 contrib/ssh-copy-id /usr/bin
install -v -m644 contrib/ssh-copy-id.1 /usr/share/man/man1
install -v -m755 -d /usr/share/doc/openssh-10.5p1
install -v -m644 INSTALL LICENCE OVERVIEW README* /usr/share/doc/openssh-10.5p1

install -d -m0700 /etc/ssh/authorized_keys
install -m0600 /root/flowlfs-authorized-key.pub /etc/ssh/authorized_keys/root
rm -f /root/flowlfs-authorized-key.pub

sed -i 's|^AuthorizedKeysFile.*|AuthorizedKeysFile /etc/ssh/authorized_keys/%u|' \
  /etc/ssh/sshd_config

cat >> /etc/ssh/sshd_config <<'EOF'

# FlowLFS callback boundary: public-key access only.
PermitRootLogin prohibit-password
PasswordAuthentication no
KbdInteractiveAuthentication no
UseDNS no
EOF

# Unlock root with an unrecoverable random password while all SSH password
# mechanisms remain disabled. Console password access is intentionally absent.
if ! test -e /etc/shadow; then
  pwconv
fi
root_password=$(openssl rand -hex 48)
usermod -p "$(openssl passwd -6 "$root_password")" root
unset root_password
chage -d "$(date --utc +%Y-%m-%d)" root
root_shadow=$(getent shadow root | cut -d: -f2)
case "$root_shadow" in
  ''|'!'*|'*'*)
    echo 'root account remained locked after callback provisioning' >&2
    exit 1
    ;;
esac
unset root_shadow
ssh-keygen -A

cat > /etc/systemd/system/sshd.service <<'EOF'
[Unit]
Description=OpenSSH server daemon
After=network.target

[Service]
ExecStartPre=/usr/sbin/sshd -t
ExecStart=/usr/sbin/sshd -D -e
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

cat > /etc/systemd/system/flowlfs-callback.service <<'EOF'
[Unit]
Description=Announce the FlowLFS callback boundary
After=sshd.service
Requires=sshd.service

[Service]
Type=oneshot
ExecStart=/usr/bin/echo FLOWLFS_CALLBACK READY phone=ssh://127.0.0.1:2222 host=%H
StandardOutput=tty
TTYPath=/dev/ttyS0
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

systemctl enable sshd.service flowlfs-callback.service
/usr/sbin/sshd -t
date --utc +'%Y-%m-%dT%H:%M:%SZ' > "$marker"
rm -rf /sources/openssh-10.5p1
echo "===== PASS callback-openssh $(date --utc +'%Y-%m-%dT%H:%M:%SZ') ====="
