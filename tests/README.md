 Tests for the cc2538
======================


 uhub-usense-coap.sh
---------------------

A simple test which runs on uHub and uSense.

This test setup two boards (sdk and cc2538), flash the chips and run uHub
python script.
The purpose of the test is to blink some leds via a COAP request to check if
the 6lowpan link is working between the different motes.
You need the coap_request.py program from the uHub repo for that.

The uSense need to be build with DEBUG option to enable leds.

TODO:
- Configure the script with program arguments.
- Find out how to interact with software in uHub repo.
- Check errors.



