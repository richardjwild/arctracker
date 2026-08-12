Yes, that is broadly the correct lifecycle. I would express it as three nested lifetimes:

```text
PortAudio lifetime
└── Pa_Initialize()
    ├── device enumeration
    ├── stream lifetime
    │   ├── Pa_OpenStream()
    │   ├── Pa_GetStreamInfo()        optional
    │   ├── Pa_StartStream()
    │   ├── Pa_WriteStream() repeatedly
    │   ├── Pa_StopStream()/Pa_AbortStream()
    │   └── Pa_CloseStream()
    └── Pa_Terminate()
```

PortAudio supports either callback operation or blocking `Pa_ReadStream`/`Pa_WriteStream`; your sequence is the correct one for its blocking interface. ([PortAudio][1])

A few refinements matter.

### Application startup

```c
Pa_Initialize();

Pa_GetDeviceCount();

for each device:
    Pa_GetDeviceInfo(device_index);
```

That is right. `Pa_GetDeviceInfo()` returns information including channel capabilities and default latency values, so this is where you construct the list offered to the UI. ([PortAudio][2])

I would initialise PortAudio once when the Arctracker audio subsystem is created and leave it initialised until shutdown. Do not initialise and terminate merely to enumerate devices, then initialise it again to open a stream.

Also, handle `Pa_GetDeviceInfo()` returning `NULL`, even for an index that was valid when enumeration began:

```c
const PaDeviceInfo *info = Pa_GetDeviceInfo(index);
if (info == NULL)
    continue;
```

### Starting audio on the selected device

You normally do need the `PaDeviceInfo` again because it provides values useful for constructing `PaStreamParameters`, particularly the device’s suggested output latency:

```c
const PaDeviceInfo *device_info =
    Pa_GetDeviceInfo(selected_device);

PaStreamParameters output = {
    .device = selected_device,
    .channelCount = 2,
    .sampleFormat = paFloat32,
    .suggestedLatency =
        device_info->defaultLowOutputLatency,
    .hostApiSpecificStreamInfo = NULL,
};

Pa_OpenStream(
    &stream,
    NULL,
    &output,
    sample_rate,
    frames_per_buffer,
    paNoFlag,
    NULL,       // NULL callback means blocking I/O
    NULL
);

const PaStreamInfo *stream_info =
    Pa_GetStreamInfo(stream);

Pa_StartStream(stream);
```

`Pa_GetStreamInfo()` belongs after a successful `Pa_OpenStream()`. It describes the actual opened stream, including its input/output latency and sample rate, and is useful precisely because the host may not grant exactly what you requested. ([PortAudio][3])

Then your writer thread may repeatedly call:

```c
Pa_WriteStream(stream, frames, frame_count);
```

until stopped. A `NULL` callback supplied to `Pa_OpenStream()` is what selects blocking read/write mode. ([PortAudio][4])

### Changing device

The safe sequence is:

```text
Tell the audio-producing/writer thread to stop using the stream
↓
Pa_StopStream() or Pa_AbortStream()
↓
Pa_CloseStream()
↓
resolve the newly selected device
↓
Pa_OpenStream()
↓
Pa_GetStreamInfo() optionally
↓
Pa_StartStream()
↓
resume writer thread
```

The first step is crucial. You must ensure no thread can still be inside or about to call `Pa_WriteStream()` while another thread closes the stream.

Use:

```c
Pa_StopStream(stream);
```

for an orderly stop. For callback streams, PortAudio distinguishes this from `Pa_AbortStream()`: stop allows pending audio to finish, while abort aims to halt promptly. The distinction is still conceptually useful when choosing how switching or shutdown should feel. ([PortAudio][3])

For changing devices, you may prefer `Pa_AbortStream()` if you want the switch to happen promptly rather than waiting for queued audio. For normal playback stop, `Pa_StopStream()` is probably the natural choice.

After either successful stop/abort—or after dealing appropriately with its error—call:

```c
Pa_CloseStream(stream);
stream = NULL;
```

`Pa_CloseStream()` is required to release stream resources; stopping alone does not close it. ([PortAudio][3])

### Failure while switching devices

This is the UX wrinkle worth planning now.

Suppose the user selects a device that has disappeared or cannot support your requested stream parameters. You will already have closed the old stream before discovering that `Pa_OpenStream()` fails.

A reasonable recovery policy is:

1. Try the selected device.
2. If that fails, try `Pa_GetDefaultOutputDevice()`.
3. If that fails, leave audio unavailable and report the error.
4. Keep the module and UI alive.

You could try opening the new stream before closing the old one, but some host APIs or devices may not permit both streams simultaneously, and it complicates ownership considerably. I would favour a simple close-and-reopen transaction with fallback.

### Device enumeration is not necessarily permanent

I would not treat the startup device list as immutable. USB interfaces, Bluetooth devices and aggregate devices may appear or disappear during the application’s lifetime.

The simplest first implementation is an explicit **Refresh devices** action that reruns:

```c
Pa_GetDeviceCount();
Pa_GetDeviceInfo(...);
```

when the audio settings dialog opens, or when the user asks. You do not need to build hot-plug notification machinery for MVP.

And as mentioned earlier, do not persist a bare PortAudio device index as though it were permanent. Use it only for the current enumeration. Persist enough descriptive information—probably host API plus device name—to resolve the preference against a fresh enumeration next time.

### Application shutdown

Your shutdown sequence should be:

```text
stop writer/player activity
Pa_StopStream() or Pa_AbortStream()
Pa_CloseStream()
Pa_Terminate()
```

Only call `Pa_Terminate()` after every PortAudio stream is closed. PortAudio’s API lifecycle is explicitly initialise, use devices/streams, close streams, then terminate. ([PortAudio][3])

So yes, your proposed shape is right. I would summarise the ownership rule as:

> `Pa_Initialize()` owns the entire PortAudio session; each selected device creates one subordinate stream lifetime; changing devices destroys that stream and creates another without terminating PortAudio.

For Arctracker, that probably means your PortAudio backend eventually owns something like:

```c
typedef struct
{
    bool portaudio_initialized;
    PaStream *stream;
    PaDeviceIndex selected_device;
    PaTime actual_output_latency;
} portaudio_state_t;
```

with one function responsible for the complete stream replacement operation. That keeps the delicate stop–close–open–start sequence in one place rather than spreading it across UI commands and playback code.

[1]: https://portaudio.com/?utm_source=chatgpt.com "PortAudio - an Open-Source Cross-Platform Audio API"
[2]: https://portaudio.com/docs/v19-doxydocs/querying_devices.html?utm_source=chatgpt.com "Enumerating and Querying PortAudio Devices"
[3]: https://portaudio.com/docs/v19-doxydocs/portaudio_8h.html?utm_source=chatgpt.com "portaudio.h File Reference"
[4]: https://portaudio.com/docs/v19-doxydocs/blocking_read_write.html?utm_source=chatgpt.com "Blocking Read/Write Functions"

>> So if I call the engine to get the available audio devices, the UI shows them to the user, and the user selects a different one, the time between querying the devices and choosing a device will easily be seconds or longer. That is more than enough time for the list of devices to go stale. This suggests I cannot count on the device index alone. Should I confirm the device name also, to verify it has not become unavailable or its index changed in the mean time?

Yes, you should treat the device index as a short-lived handle, valid only for the particular enumeration you performed.

When the user confirms their choice, I would re-query the index:

```c
const PaDeviceInfo *info = Pa_GetDeviceInfo(selected_index);
```

Then verify at least:

* `info != NULL`;
* it still supports two output channels;
* its name matches the device the user selected.

PortAudio only documents the index as valid within the current range returned by `Pa_GetDeviceCount()`, and `Pa_GetDeviceInfo()` returns `NULL` when the index is no longer valid. It does not promise that indices are durable identifiers. ([PortAudio][1])

However, I would go slightly further than just checking the name. Device names are not guaranteed unique. A machine could easily have two devices called something generic such as “USB Audio Device”, especially across different PortAudio host APIs.

So the UI-facing identity should ideally contain:

```ts
type AudioDevice = {
  index: number;
  name: string;
  hostApi: string;
};
```

Then when applying the selection, verify both the name and host API against the device currently occupying that index.

Conceptually:

```c
bool selected_device_still_matches(
    PaDeviceIndex index,
    const char *expected_name,
    PaHostApiIndex expected_host_api)
{
    const PaDeviceInfo *info = Pa_GetDeviceInfo(index);

    return info != NULL
        && info->maxOutputChannels >= 2
        && info->hostApi == expected_host_api
        && strcmp(info->name, expected_name) == 0;
}
```

If it no longer matches, re-enumerate and try to locate the device by the same host API and name. `PaDeviceInfo` includes the host API index, so that pairing is readily available. ([PortAudio][2])

The selection process would therefore be:

```text
UI sends:
    old PortAudio index
    device name
    host API identity

Engine:
    inspect current device at old index
    if identity still matches:
        use it
    otherwise:
        re-enumerate and search for matching identity
    if no match:
        report that the device is no longer available
```

This same mechanism also gives you a reasonable basis for persisted configuration. Store the descriptive identity, not the PortAudio index:

```ts
type AudioDevicePreference = {
  name: string;
  hostApi: string;
};
```

On the next launch, enumerate afresh and resolve that preference to the current index. If it cannot be resolved, fall back to the default output device.

There is still an unavoidable race between the final verification and `Pa_OpenStream()`: the device could disappear one millisecond later. You cannot eliminate that merely by checking more carefully. `Pa_OpenStream()` must remain the authoritative test, and failure there must be handled cleanly.

So the robust rule is:

> Use index, name, and host API to detect likely staleness; trust only a successful `Pa_OpenStream()`.

The index is useful for immediate selection, the descriptive identity is useful for re-resolution, and opening the stream is the final proof that the device is actually usable.

[1]: https://portaudio.com/docs/v19-doxydocs/querying_devices.html?utm_source=chatgpt.com "Enumerating and Querying PortAudio Devices"
[2]: https://portaudio.com/docs/v19-doxydocs-dev/structPaDeviceInfo.html?utm_source=chatgpt.com "PaDeviceInfo Struct Reference"

>> Once I've returned the list of devices to the UI, should I consider the device index meaningless now, and rely on the name and host API alone? That would mean, when the user selects an output device, I pass the name to the back end, and it once again enumerates the available devices, and selects the one with a name that matches, or falls back to the default device if none do.

Almost—but I wouldn’t throw away the index entirely, and I wouldn’t rely on the name alone.

PortAudio’s cross-platform API does not expose a guaranteed persistent device identifier. What it does expose in `PaDeviceInfo` is the current index, device name, host API, channel capabilities, latency defaults, and default sample rate. Device names are not guaranteed unique, particularly if multiple host APIs expose the same physical device or two identical USB devices are attached. ([PortAudio][1])

I’d have the UI return a composite selection:

```ts
type AudioDeviceSelection = {
  index: number;
  name: string;
  hostApi: number;
};
```

Then the engine follows this policy:

1. Check the device currently at `index`.
2. If its name and host API still match, use that index.
3. Otherwise enumerate again and search for the same `name + hostApi`.
4. If there is exactly one match, use its new index.
5. If there is no unambiguous match, fall back to the default device—or report that the chosen device disappeared, depending on the UX you prefer.
6. In every case, `Pa_OpenStream()` remains the final authority: the device can disappear even after successful validation.

Conceptually:

```c
static PaDeviceIndex resolve_output_device(
    const PaDeviceIndex previous_index,
    const char *expected_name,
    const PaHostApiIndex expected_host_api)
{
    const PaDeviceInfo *info = Pa_GetDeviceInfo(previous_index);

    if (info != NULL
        && info->maxOutputChannels >= 2
        && info->hostApi == expected_host_api
        && strcmp(info->name, expected_name) == 0)
    {
        return previous_index;
    }

    const PaDeviceIndex count = Pa_GetDeviceCount();

    for (PaDeviceIndex index = 0; index < count; index++)
    {
        info = Pa_GetDeviceInfo(index);

        if (info != NULL
            && info->maxOutputChannels >= 2
            && info->hostApi == expected_host_api
            && strcmp(info->name, expected_name) == 0)
        {
            return index;
        }
    }

    return Pa_GetDefaultOutputDevice();
}
```

I would therefore describe the index as **ephemeral but useful**, not meaningless. It provides a cheap fast path for the exact enumeration the UI saw. The descriptive identity lets you recover when that index has gone stale.

For persisted configuration, don’t store the index. Store at least:

```ts
type AudioDevicePreference = {
  name: string;
  hostApiName: string;
};
```

Using the host API’s name rather than its numeric index is preferable across application sessions, because the numeric host API index is another enumeration value. PortAudio exposes the device’s host API and provides per-host device enumeration, but it does not promise a durable global device ID. ([PortAudio][2])

There remains one awkward corner case: two identical devices with the same name under the same host API. PortAudio’s portable API cannot reliably distinguish them after indices change. You could include more descriptive fields as heuristics, but they still would not constitute a guaranteed identity. For an MVP, `index + name + host API`, followed by graceful fallback, is the proportionate design.

So: **send all three back from the UI; validate the index first; re-resolve by name and host API if necessary; never trust name alone.**

[1]: https://files.portaudio.com/docs/v19-doxydocs/structPaDeviceInfo.html?utm_source=chatgpt.com "PaDeviceInfo Struct Reference"
[2]: https://portaudio.com/docs/v19-doxydocs/portaudio_8h.html?utm_source=chatgpt.com "portaudio.h File Reference"
