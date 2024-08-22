*** Test Cases ***
Should Boot
    Execute Command           i @${CURDIR}/cubesat.resc

    Create Terminal Tester    sysbus.mmuart1  30  machine=Processing Module

    IF  %{HALT=False}
        Execute Command  u54_2 IsHalted true
        Execute Command  u54_3 IsHalted true
        Execute Command  u54_4 IsHalted true
    END

        Start Emulation
        Sleep                     ${{__import__("random").random()*5}}

    Execute Command           i2c2.sht4xd9b52d72767ad Temperature 40  machine=OBC Controller
    Execute Command           set_voltage 3000
    Wait For Prompt On Uart   buildroot login
