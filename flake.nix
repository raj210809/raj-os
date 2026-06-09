{
  description = "OS Development Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      inherit system;
    };
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        nasm
        gcc
        binutils
        qemu
        gdb
      ];

      shellHook = ''
        echo "OS Development Environment Loaded"
        echo "NASM: $(nasm -v)"
        echo "QEMU: $(qemu-system-x86_64 --version | head -n1)"
        echo "GDB:  $(gdb --version | head -n1)"
        echo ""
        echo "  make run          boot normally (VGA window + serial)"
        echo "  make debug        GDB + QEMU window (serial in terminal)"
        echo "  make gdb-server   two-terminal: start QEMU (terminal 1)"
        echo "  make gdb-connect  two-terminal: connect GDB (terminal 2)"
        echo ""
        echo "  make run: serial output (kmain messages) -> terminal, VGA -> QEMU window"
        echo "  make debug: CPU paused until you type 'c' in GDB (window stays black)"
        echo "  Tip: DEBUG_HEADLESS=1 make debug  |  GDB_AUTO_CONTINUE=1 make gdb-connect"
      '';
    };
  };
}