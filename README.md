# auto-safety-hil-ecu

A learning project: a simulated car on a PC talks to a real microcontroller over a car network. The microcontroller acts as a safety computer. It checks what it hears and decides when to step in. The simulator is also a place to train driving software, with the safety computer always having the final say.

## How data flows

```mermaid
flowchart LR
    subgraph PC["PC (Windows)"]
        World["3D car simulator<br/>(car, pedestrians, sensors)"]
        Driver["Driver software<br/>(scripted, classic or trained)"]
    end
    Adapter["USB-to-CAN adapter"]
    Cable(["CAN bus cable<br/>(two twisted wires)"])
    Transceiver["CAN transceiver chip"]
    subgraph STM32["STM32 board (NUCLEO-G474RE, FreeRTOS)"]
        Check{"Check each message:<br/>intact? new? on time? sensible?"}
        Decide["Decide: allow, limit or brake"]
        Distrust["Distrust it: slow down safely"]
    end

    World -- "sensor readings, over USB" --> Adapter
    Driver -- "driving request, over USB" --> Adapter
    Adapter --- Cable
    Cable --- Transceiver
    Transceiver -- "CAN frames" --> Check
    Check -- "passes" --> Decide
    Check -- "fails" --> Distrust
    Decide -- "final command, back over CAN" --> Transceiver
    Distrust -- "final command, back over CAN" --> Transceiver
    Adapter -- "final command, over USB" --> World
```

The driver software on the PC only asks. The STM32 always decides what the car actually does.

> **Not a real safety system.** This is for learning only. It borrows ideas from car safety engineering, but claims no safety standard, and it controls only a simulated car.

## License
MIT, see [LICENSE](LICENSE). Third-party code keeps its own license.
