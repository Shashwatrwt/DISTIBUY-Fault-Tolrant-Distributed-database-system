# Fault-Tolerant Distributed Database System


A distributed database system designed to remain available and consistent when individual nodes fail.

## Status

This project is currently under development.

## Planned Features

- Data replication across multiple nodes
- Fault detection and recovery
- Consistent data operations
- Node and cluster management
- Client access to the distributed datastore

## Getting Started

Compile and run the current C++ node with MinGW:

```powershell
g++ -Wall -Wextra core/src/main.cpp -o core/src/main.exe
.\core\src\main.exe
```

The current example starts node 1 on `127.0.0.1:5000` with node 2 configured
as its peer on `127.0.0.1:5001`.

## Contributing

Contributions are welcome. Please open an issue to discuss a proposed change before submitting a pull request.

## License

No license has been selected yet.
