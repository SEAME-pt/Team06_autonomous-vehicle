#!/bin/bash

echo "Iniciando configuração do CAN para Jetson Nano..."

# Cria diretório se não existir
echo "Criando diretório..."
sudo mkdir -p /boot/overlays

# Cria arquivo dtbo com configuração simplificada
echo "Criando arquivo overlay..."
sudo tee /boot/overlays/mcp2515-can0.dts > /dev/null << 'EOL'
/dts-v1/;
/plugin/;

/ {
    compatible = "nvidia,tegra210";
    fragment@0 {
        target = <&spi0>;
        __overlay__ {
            status = "okay";
            mcp2515@0 {
                compatible = "microchip,mcp2515";
                reg = <0>;
                spi-max-frequency = <10000000>;
                status = "okay";
            };
        };
    };
};
EOL

# Compila o device tree
echo "Compilando device tree..."
sudo dtc -@ -I dts -O dtb -o /boot/overlays/mcp2515-can0.dtbo /boot/overlays/mcp2515-can0.dts

# Adiciona referência ao overlay no extlinux.conf
echo "Configurando boot..."
sudo sed -i 's/^APPEND.*/& devicetree_overlay=\/boot\/overlays\/mcp2515-can0.dtbo/' /boot/extlinux/extlinux.conf

# Configura o SPI
echo "Configurando SPI..."
echo "spi-tegra114" | sudo tee -a /etc/modules
echo "spidev" | sudo tee -a /etc/modules

# Carrega módulos necessários
echo "Carregando módulos..."
sudo modprobe spi-tegra114
sudo modprobe can
sudo modprobe can_raw
sudo modprobe can_dev
sudo modprobe mcp251x

# Adiciona módulos ao boot
echo "Configurando módulos para carregar no boot..."
echo "can" | sudo tee -a /etc/modules
echo "can_raw" | sudo tee -a /etc/modules
echo "can_dev" | sudo tee -a /etc/modules
echo "mcp251x" | sudo tee -a /etc/modules

# Configura regras udev para CAN
echo "Configurando regras udev..."
sudo tee /etc/udev/rules.d/99-can.rules > /dev/null << 'EOL'
KERNEL=="can*", NAME="can%n", GROUP="dialout", MODE="0660"
EOL

# Configura o script de inicialização do CAN
echo "Criando script de inicialização..."
sudo tee /etc/systemd/system/can0-setup.service > /dev/null << 'EOL'
[Unit]
Description=Setup CAN0 interface
After=multi-user.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/sbin/ip link set can0 up type can bitrate 500000

[Install]
WantedBy=multi-user.target
EOL

# Habilita o serviço
echo "Habilitando serviço de inicialização..."
sudo systemctl enable can0-setup.service

echo "Configuração concluída! Por favor, reinicie o sistema com: sudo reboot"