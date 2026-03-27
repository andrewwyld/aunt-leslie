# Aunt Leslie
A chorus with synchronized low-pass filter to emulate a Leslie speaker.

## Concept
The Leslie speaker passes the sound output of a driver through a rotating element. The rotation creates a pleasing variation in the sound. It was originally designed to work with the Hammond organ, and is part of the characteristic classic Hammond sound.

In the Leslie 760, which is the unit I used for inspiration, there are two rotors: a treble rotor with two horns, and a bass rotor with a wedge-shaped reflector inside a rotating cylinder.

I wanted to have a real Leslie speaker because it would allow me to place microphones to give a stereo chorus (this worked very well).

### Specifics of the Leslie

#### Treble rotor
The treble rotor has a radius of about 204mm, and rotates at about 36RPM in the chorale setting and 380RPM in the tremolo setting. When shifting between settings, the rotor is accelerated or braked at a constant rate, taking about a second to adjust speed.

#### Bass rotor
TODO

## Phase shift mathematics
The input signal will be shifted by a variable amount depending on a function designed to mimic the audio path through a Leslie speaker. A classic BBD chorus simply uses a sine function, but a lovely characteristic feature of a real Leslie is that the delay is slightly more complex, leading to an asymmetrical throb, particularly in the tremolo setting.

### Treble rotor geometry
![the geometry of the treble rotor, with a microphone](./README_img/geometry_treble_horn_mic.jpg)
The treble rotor consists of two horns, facing in opposite directions, rotating around a common centre. A driver at the centre passes sound into the throat of both horns, and it is projected up each to the end of the horn.

Let the treble rotor radius be $r$.

A microphone is placed at a distance from the centre of the treble rotor. For reasons that will shortly be obvious, let this distance be a multiple $k$ of the treble rotor radius.

The treble rotor rotates clockwise as seen from above. Let the angle of rotation at a given time, with respect to the axis from the centre of the treble rotor to the microphone, be $\theta$.

The sound first has to travel a distance $r$ up the treble horn, and it then leaves at the end and travels a further distance to the microphone.

That distance is the hypotenuse of a right triangle. One side of the right triangle is $rsin\theta$, the sideways displacement of the treble horn. The other side is $kr - rcos\theta$, that is, the total distance from the centre of the treble horn to the microphone, less the longitudinal displacement of the treble horn. Thus, the hypotenuse of the right triangle has length $r\sqrt{(k - \cos\theta)^2 + \sin^2\theta}$.

Thus the total distance travelled is:

$r[1 + \sqrt{(k - \cos\theta)^2 + \sin^2\theta}]$

And hopefully now it's obvious why I chose to make the microphone distance a multiple of the treble rotor radius!

#### Accounting for both horns and stereo input
Since there are two horns, we will model two delay lines and sum their outputs.

Since the plugin will have a stereo input, this gives us an opportunity to choose what signal we send to each horn. We will have three switchable modes:

- summed stereo to both horns
- left input to one horn, right input to the other
- stereo difference to one horn, inverted stereo difference to the other

Who knows what that'll sound like, but I *wanted* to use stereo difference with my real Leslie and obviously, physically, I can't. So why not?

### Bass rotor geometry
The bass rotor geometry can be considered to work identically to the treble rotor, except that it only has one wedge to the treble rotor's two horns.

### Stereo microphone placement
For my actual recording of the Leslie 760, I placed the microphones at right angles. Therefore, for this first version of the plugin, we will do the same. Using the original expression for the right microphone, we obtain the expression for the left microphone by substituting $\sin$ for $\cos$ and $-\cos$ for $\sin$. This means the stereo microphone geometries will be:

Right: $r[1 + \sqrt{(k - \cos\theta)^2 + \sin^2\theta}]$

Left: $r[1 + \sqrt{(k - \sin\theta)^2 + \cos^2\theta}]$

A more advanced future version will allow for a variable phase shift between microphones, and indeed possibly more than two microphones if we're getting excitable.

## Filter mathematics
For now we simply use:

Right: $cos\theta$

Left: $sin\theta$

Using the same definition of $\theta$ above. We will use this to modulate the cutoff frequency of a simple discrete filter, finding a good level experimentally.

A more sophisticated analysis, involving the tendency for higher frequency sounds to be more directional, will come in a future version.

## References

Much of the electrical and mechanical detail:
https://synthfool.com/docs/Leslie/Leslie_760_User_And_Servicemanual/

Rotor speeds:
https://organforum.com/forums/forum/electronic-organs-midi/leslies-tone-cabinets-speakers-accessories/41662-leslie-rotor-speeds