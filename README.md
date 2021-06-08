> As of 2018 development on this project had ceased cause I moved into a building with a real thermostat.
> Arduino doesn't change much, so the code should still work. However, I am archiving the project so it's
> clear that this isn't supported.

# Hacky Nest Alternatives

I live in a kinda run-down tenement building with window heater units.
Since I was definitely not going to be able to install Nest thermostats
in my building, I decided to hack my own together using relays, temperature sensors,
and a few [Particle Photon boards](https://www.particle.io/).

This repo has the code I use to power the two type of units in my apartment. I have
thermostat units on my heaters (though the code also allows a boolean flip to
support being attached to an AC unit), and a sensor base to give extra readings from
the middle of the apartment in case the sensors on the heaters fail or give spurious
readings.

Feel free to use this code or reach out with any questions about my setup.
