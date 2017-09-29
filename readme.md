# Midas Project

---

### Env

| name        | description   |  default  |
| :--------:   | :-----:  | :----:  |
| MIDAS_LOG_LEVEL  |  |   INFO     |
| MIDAS_CPU_AFFINITY | | "" |


### Option

| short | long    | description   |  default  |
| :---: | :----:  | :-----:       | :----:    |
| -h    | -help   | print help info |         |


### how to compile

```bash
$ mkdir install
$ cd install
$ cmake ..
$ make
```

### Code Style

- function use underscore style, like do_something()
- class, struct, enum camel case and start with uppercase, like Tree
- typedef use camel case style, like class name
- variable use camel case style, like isGood
- class definition, data first then function