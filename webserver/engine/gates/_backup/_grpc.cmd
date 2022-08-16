echo off
::Для проекта GrpcGate, который в общей папке TFS Servers\GrpcGate
::SET PROTO_PATH=../../../../../Servers/GrpcGate/protos

::Для проекта, который лежит в c:\\<folder>\\Servers\\GrpcGate
::SET PROTO_PATH=../../../../../../Kotmi-dev/mainline/develop/Servers/GrpcGate/protos

::Для проекта RpcGate, который в папке TFS c:\\<folder>\\GrpcGate\\
::SET PROTO_PATH=../../../../../../Kotmi-dev/mainline/develop/GrpcGate/protos

::Для проекта GrpcGate, который в общей папке TFS GrpcGate

set VCPKG_PATH=c:\vcpkg\installed\x64-windows
set PROTOC_PATH=%VCPKG_PATH%\tools\protobuf
set GOPATH=c:\Users\Public\go
set Path=%Path%;%GOPATH%/bin

SET PROTO_PATH=../../../../GrpcGate/protos
rd /s /q "./scd/scdgate"
mkdir "./scd/scdgate"
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go_out=paths=source_relative:./scd/scdgate %PROTO_PATH%/scdgate.proto 
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go-grpc_out=./scd/scdgate %PROTO_PATH%/scdgate.proto 

SET PROTO_PATH=../../../../Servers/LicenseServer/protos
rd /s /q "./license/licgate"
mkdir "./license/licgate"
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go_out=paths=source_relative:./license/licgate %PROTO_PATH%/licgate.proto 
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go-grpc_out=./license/licgate %PROTO_PATH%/licgate.proto 
