//
// Copyright (c) 2010-2024 Antmicro
//
// This file is licensed under the MIT License.
// Full license text is available in 'licenses/MIT.txt'.
//
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Peripherals.CPU;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class PowerSequencer : IPeripheral, IGPIOReceiver
    {
        public PowerSequencer(IMachine machine, ICPU cpu1 = null, ICPU cpu2 = null, ICPU cpu3 = null, ICPU cpu4 = null, ICPU cpu5 = null)
        {
            this.machine = machine;
            this.cpus = new List<ICPU>();
            if(cpu1 != null)
            {
                cpus.Add(cpu1);
            }
            if(cpu2 != null)
            {
                cpus.Add(cpu2);
            }
            if(cpu3 != null)
            {
                cpus.Add(cpu3);
            }
            if(cpu4 != null)
            {
                cpus.Add(cpu4);
            }
            if(cpu5 != null)
            {
                cpus.Add(cpu5);
            }

            Reset();
        }

        public void Reset()
        {
            foreach(var cpu in cpus)
            {
                cpu.IsHalted = true;
            }
        }

        public void OnGPIO(int number, bool value)
        {
            if(!value)
            {
                return;
            }

            foreach(var cpu in cpus)
            {
                cpu.IsHalted = false;
            }
        }

        private readonly IMachine machine;
        private readonly List<ICPU> cpus;
    }
}
