import fentonControl_pb2_grpc
import fentonControl_pb2
# from __future__ import print_function
import logging
# from google.protobuf import empty_pb2
import grpc

# import os

# path = os.getcwd()

# print(path)


def run():
    # NOTE(gRPC Python Team): .close() is possible on a channel and should be
    # used in circumstances in which the with statement does not fit the needs
    # of the code.
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = fentonControl_pb2_grpc.FentonControlStub(channel)
        response = stub.SayHello(fentonControl_pb2.HelloRequest(name='you'))
        response = stub.SayHello(fentonControl_pb2.HelloRequest(name='you2'))
        # print(fentonControl_pb2)
        # display(0)
        stub.StopSim(fentonControl_pb2.Empty())
    print("Greeter client received: " + response.message)


if __name__ == '__main__':
    logging.basicConfig()
    run()
