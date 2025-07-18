# PulseAudio Sink Toggle Tool

Pats is a command-line tool for managing PulseAudio audio sinks that allows you to
list available audio outputs and toggle between them seamlessly.

## Features

| Feature | Description |
|---------|-------------|
| **List audio sinks** | Display all available PulseAudio sinks with active status |
| **Toggle between sinks** | Switch between available audio outputs |

## How to use

> [!IMPORTANT]
> Make sure you have the required dependencies installed before
> building the project.

First, ensure you have the necessary development libraries installed:

```sh
# On Void Linux
sudo xbps-install -S pulseaudio-devel

# On Debian/Ubuntu
sudo apt-get install libpulse-dev

# On Fedora/RHEL
sudo dnf install pulseaudio-libs-devel

# On Arch Linux
sudo pacman -S pulseaudio
```

Build the project:

```sh
make
sudo make install
```

### Command line options

The tool supports several command line options:

```sh
pats [OPTIONS]

Options:
  -l, --list     List all available audio sinks
  -t, --toggle   Toggle between available audio sinks
  -h, --help     Show this help message
```

### Examples

List all available audio sinks:
```sh
pats --list
```

Toggle to the next available audio sink:
```sh
pats --toggle
```

Short option syntax:
```sh
pats -l    # List sinks
pats -t    # Toggle sinks
```

## Building from source

The project uses a simple Makefile for building. Available targets:

```sh
make                  # Build the executable (default)
make clean            # Remove build artifacts
sudo make install     # Install to /usr/local/bin/
sudo make uninstall   # Remove from /usr/local/bin/
```

### Debugging

Enable verbose output by modifying the source code or using
debugging tools like `gdb`:

```sh
gdb ./pats
```

## License

[pats](https://github.com/mitjafelicijan/pats) was written by [Mitja
Felicijan](https://mitjafelicijan.com) and is released under the BSD
two-clause license, see the LICENSE file for more information.. 