# Chord
Chord Protocol Implementation

My implementation of the [Chord Scalable Peer-to-Peer Lookup Protocol](https://pdos.csail.mit.edu/papers/ton:chord/paper-ton.pdf), written in C (with eventual visualizer).

## Build Instructions
Simply run ```make``` to create the main binary.
For testing (currently) navigate to the 'test' directory:
```cd test```
And run ```make``` again to generate the test_program binary.
With main running, and **port 1234** specified, simply run the test with ```./test_program```.

## Options
- -p <Number> Port that the Chord node will bind to and listen on 0 < port <= 65535
- --sp <Number> Time in deciseconds between invocations of 'stabilize' 1 <= stabilize period <= 600
- --ffp <Number> Time in deciseconds between invocations of 'fix_fingers' 1 <= fix fingers period <= 600
- --cpp <Number> Time in deciseconds between invocation of 'check_predecessor' 1 <= check predecessor period <= 600
- --ja <String> The IP address of a Chord node whose ring this node will join. Must be an ASCII string (e.g. 128.10.134.55) and must be specified if --jp is specified
- --jp <Number> The port of the Chord node whose ring this node will join 0 < port <= 65535

Example usage:

```./main -p 1234 --sp 5 --ffp 6 --cpp 7 ```

To join an existing Chord ring:

```./main -p 1234 --ja 128.10.134.55 --jp 4180 --sp 5 --ffp 6 --cpp 7 ```


README will continue to expand with new additions!

