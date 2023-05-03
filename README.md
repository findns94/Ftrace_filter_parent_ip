# Use parent_ip to filter parent function in ftrace

Make sure these kernel features are set

- MEMCG
- CONFIG_FUNCTION_TRACER

## Usage

```
export KDIR=/lib/modules/`uname -r`/build
make
```
