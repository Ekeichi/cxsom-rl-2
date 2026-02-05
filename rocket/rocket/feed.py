import pycxsom as cx

hostname = 'localhost'
port = 10000

print("Starting dataset feeding...")
if cx.client.ping(hostname, port):
    print('Something went wrong.')
else:
    print('ping successful.')
