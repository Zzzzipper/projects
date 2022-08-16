echo off

::Для проекта GrpcGate, который в общей папке TFS GrpcGate

set VCPKG_PATH=c:\vcpkg\installed\x64-windows
set PROTOC_PATH=%VCPKG_PATH%\tools\protobuf
set GOPATH=c:\Users\Public\go
set Path=%Path%;%GOPATH%/bin

SET PROTO_PATH=../../../../GrpcGate/protos
rd /s /q "./grpc/grpcgate"
mkdir "./grpc/grpcgate"
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go_out=paths=source_relative:./grpc/grpcgate %PROTO_PATH%/grpcgate.proto 
%PROTOC_PATH%/protoc -I=%PROTO_PATH% -I=%VCPKG_PATH%/include --go-grpc_out=./grpc/grpcgate %PROTO_PATH%/grpcgate.proto 

pause