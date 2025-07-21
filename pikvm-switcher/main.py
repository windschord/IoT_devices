from machine import Pin
from time import sleep

# working check led
LED_PIN = Pin(25, Pin.OUT)

# to 74HC4052
MULTIPLEXER_CNT = [
    Pin(15, Pin.OUT, value=0), # A
    Pin(14, Pin.OUT, value=0)  # B
]

# to kvm
KVM_SW = [
    Pin(13, Pin.OUT, value=1),
    Pin(12, Pin.OUT, value=1),
    Pin(11, Pin.OUT, value=1),
    Pin(10, Pin.OUT, value=1)
]

# from kvm
KVM_STATUS = [
    Pin(9, Pin.IN, Pin.PULL_DOWN),
    Pin(8, Pin.IN, Pin.PULL_DOWN),
    Pin(7, Pin.IN, Pin.PULL_DOWN),
    Pin(6, Pin.IN, Pin.PULL_DOWN)
]

# to pikvm gpio
SELECTED_PC = [
    Pin(16, Pin.OUT, value=0),
    Pin(17, Pin.OUT, value=0),
    Pin(18, Pin.OUT, value=0),
    Pin(19, Pin.OUT, value=0)
]
    
# from  pikvm gpio
SELECT_SW = [
    Pin(20, Pin.IN, Pin.PULL_DOWN),
    Pin(21, Pin.IN, Pin.PULL_DOWN),
    Pin(22, Pin.IN, Pin.PULL_DOWN),
    Pin(26, Pin.IN, Pin.PULL_DOWN)
]



def select_multiplexer(target_number):
    if target_number == 0:
        MULTIPLEXER_CNT[0].value(False)
        MULTIPLEXER_CNT[1].value(False)
        print(f"select {target_number}: A={MULTIPLEXER_CNT[0].value()} B={MULTIPLEXER_CNT[1].value()}")
    elif target_number == 1:
        MULTIPLEXER_CNT[0].value(True)
        MULTIPLEXER_CNT[1].value(False)
        print(f"select {target_number}: A={MULTIPLEXER_CNT[0].value()} B={MULTIPLEXER_CNT[1].value()}")
    elif target_number == 2:
        MULTIPLEXER_CNT[0].value(False)
        MULTIPLEXER_CNT[1].value(True)
        print(f"select {target_number}: A={MULTIPLEXER_CNT[0].value()} B={MULTIPLEXER_CNT[1].value()}")
    elif target_number == 3:
        MULTIPLEXER_CNT[0].value(True)
        MULTIPLEXER_CNT[1].value(True)
        print(f"select {target_number}: A={MULTIPLEXER_CNT[0].value()} B={MULTIPLEXER_CNT[1].value()}")
    else:
        MULTIPLEXER_CNT_A = False
        MULTIPLEXER_CNT_B = False
        print(f"Unkonwn Number {target_number}")


def kvm_select_num():
    for i in range(4):
        if not KVM_STATUS[i].value():
            return i

    print("Cannot find sected number")
    return 1

previous_select = 0
def select_kvm_target(target_number):
    global previous_select
    kvm_select = [True, True, True, True]
    kvm_select[target_number] = False
    
    if previous_select == target_number:
        return
    
    print(f"KVM port change to {target_number}")
    
    
    
    for state, target in zip(kvm_select, KVM_SW):
        print(f"{target}: {state}")
        target.value(state)    

    sleep(0.01)

    for state, target in zip([True, True, True, True], KVM_SW):
        print(f"{target}: {state}")
        target.value(state)   
    
    previous_select = target_number
    
        

def get_select_sw(default_select):
    for i, pin in enumerate(SELECT_SW):
        if pin.value():
            return i
    return default_select

def update_selected_pc(target_number):
    return_state = [False, False, False, False]
    return_state[target_number] = True
    
    for state, target in zip(return_state, SELECTED_PC):
        print(f"{target}: {state}")
        target.value(state)
        

LED_ON = False
HEATBEAT_CNT = 0
def brinlk_heartbeat():
    global LED_ON, HEATBEAT_CNT
    if HEATBEAT_CNT > 10:
        LED_PIN.value(LED_ON)
        LED_ON = not LED_ON
        HEATBEAT_CNT = 0
    else:
        HEATBEAT_CNT += 1
        



# init
kvm_select = kvm_select_num()

# ===============================
# main logic
while True:
    print("=" * 20)
    select = get_select_sw(kvm_select)
    print(f"select {select}")

    
    select_kvm_target(select)
    select_multiplexer(select)
    
    kvm_select = kvm_select_num()
    print(f"kvm_select {kvm_select}")
    update_selected_pc(kvm_select)
    
    brinlk_heartbeat()
    sleep(0.1)

