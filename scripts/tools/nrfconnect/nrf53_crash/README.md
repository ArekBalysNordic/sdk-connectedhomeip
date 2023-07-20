Instruction on how to reproduce nRF53 network core crash using Matter Lock sample.

The zip file consists of 3 files - `nrf53_crash.py` script, `app.hex` a binary file containing a Matter sample prepared to reproduce the crash; and this `README.md` instruction.

The Python script should work on both Linux and Windows (It uses the generic library), but it was tested on Linux only. 

# Reproduction

To reproduce a crash do all of the following points:

1. Install Python in version at least 3.8
2. Install Python's library:

    ```
    $ python -m pip install pyserial
    ```

3. Install `nrfjprog`:

    https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download

4. Connected nRF53DK to the PC.
5. Check and copy Jlink SNR of the connected board:

    - Read directly from the sticker located on the nRF53DK.
    - Run:

    ```
    $ nrfjprog -i
    ```
6. Run the Python script:

    ```
    $ python nrf53_crash.py --snr <nRF53 Jlink SNR> -i <iterations> --flash
    ```

    In this command:

    - `--snr` - Jlink SNR number read out in point no. 5
    - `--i` - Number of iterations to reproduce the crash. If the crash is not reproducible, increase the number of iterations.
    - `--flash` (optional) - Flash the device with a Matter sample FW. This argument should be added once at the first usage because between the following runs of the script, there is no need to re-flash the DK
    - `--hex` (optional) - path to hex file containing a Matter application FW. By default, it is set to `./app.hex`.

    Examples of the script usage:
    
    - First usage:

    ```
    $ python nrf53_crash.py --snr 1050045314 -i 100 --flash
    ```
    
    - Each next usage:
 
    ```
    $ python nrf53_crash.py --snr 1050045314 -i 100
    ```

7. Observe the CLI output and wait for the message `"CRASH DETECTED!"`

For example, a CLI output can look like the following one:

```
$ python nrf53_crash.py --snr 1050045314 -i 100
Start testing...


 Iteration no. 0
Applying pin reset.


 NO CRASH

 Iteration no. 1
Applying pin reset.


 NO CRASH

 Iteration no. 2
Applying pin reset.


 CRASH DETECTED!
 ```

The crash is hard to reproduce. Sometimes it happens after 2 iterations, sometimes after 200... So it is worth running the script several times with different `-i` arguments to catch the crash.

To build a matter sample and prepare a hex file to reproduce a crash do the following steps:

1. Install nRFconnect SDK:

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.0/nrf/getting_started/assistant.html

2. checkout nrf revision to 2.4.0 TAG.

3. go to the matter sample directory:

    nrf/samples/matter/lock

4. build the example on nRF5340 target:

    ```
    $ west build -d nrf5340dk_nrf5340_cpuapp -p
    ```

5. Merge the output hex file with the prepared `settings.hex` file (fill the `<path to settings.hex file>` and `<path to output hex file>` arguments and run the following command from the Matter lock sample directory):


    ```
    $ ../../../zephyr/scripts/build/mergehex.py -o <path to output hex file> build/zephyr/merged_domains.hex <path to settings.hex file> 
    ```

6. Run the reproduction script as usual but provide the new hex file as an argument:

    ```
    $ python nrf53_crash.py --snr <nRF53 Jlink SNR> -i <iterations> --flash --hex <path to the new hex file>
    ```