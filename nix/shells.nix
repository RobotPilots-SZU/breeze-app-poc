{ ... }:
{
  perSystem =
    { inputs', pkgs, ... }:
    let
      inherit (inputs') breeze;

      nanopbPython = pkgs.python3.withPackages (ps: [
        ps.protobuf
        ps.grpcio-tools
      ]);
    in
    {
      devShells.default = breeze.devShells.native.overrideAttrs (old: {
        nativeBuildInputs = [
          nanopbPython
          pkgs.protobuf
        ]
        ++ (old.nativeBuildInputs or [ ]);
      });
      devShells.arm = breeze.devShells.arm.overrideAttrs (old: {
        nativeBuildInputs = [
          nanopbPython
          pkgs.protobuf
          pkgs.gcc-arm-embedded
        ]
        ++ (old.nativeBuildInputs or [ ]);
      });
    };
}
