const messages = {
  en: {
    unsavedChanges:
      "You have unsaved changes which will be lost. Proceed anyway?",
    sampleLoadFailed: "Failed to load sample",
    exportAudioFailed: "Export audio encountered an error",
    invalidTrackCount: "Number of tracks must be a number between 1 and 256.",
    invalidTempo: "Tempo must be a number between 0 and 255.",
    invalidPatternLength: "Pattern length must be a number between 1 and 1000.",
    invalidDefaultPatternLength:
      "Default pattern length must be a number between 1 and 1000.",
    invalidTranspose: "Transpose must be between -12 and 12.",
    invalidRepeatStartWithRepeatEnd:
      "Repeat start must be between 0 and repeat end.",
    invalidRepeatStartWithoutRepeatEnd:
      "Repeat start must be between 0 and sample length.",
    invalidRepeatEnd:
      "Repeat end must be between repeat start and sample length.",
    invalidLinesPerBeat: "Lines per beat must be a number between 0 and 255.",
    tempoUndefinedWithoutLinesPerBeat:
      "Lines per beat must be set for tempo to be valid.",
    moduleNameLabel: "Module Name:",
    saveModuleTitle: "Save module",
    exportAudioTitle: "Export audio",
    exportSampleTitle: "Export sample",
    authorNameLabel: "Author:",
    patternLengthLabel: "Pattern Length:",
    linesPerBeatLabel: "Lines per Beat:",
    beatsPerMinuteLabel: "Tempo (bpm):",
    defaultPatternLengthLabel: "Default Pattern Length:",
    interpolationTypeLabel: "Sample Interpolation:",
    arctrackerInterpolationType: "Arctracker (linear)",
    archimedesInterpolationType: "Archimedes/Amiga (none)",
    volumeMappingLabel: "Volume Mapping:",
    arctrackerVolumeMapping: "Arctracker/Archimedes (logarithmic)",
    amigaVolumeMapping: "Amiga (linear)",
    trackCountLabel: "Number of Tracks:",
    moduleFileFilterDescription: "Arctracker Module Files",
    audioFileFilterDescription: "Audio Samples",
    exportingAudioMessage: "Exporting audio...",
    unassignedInstrumentName: "(empty)",
    addInstrumentButtonLabel: "Add Instrument",
    loadModuleHintText: "Load module from file",
    saveModuleHintText: "Save module to file",
    bounceAudioHintText: "Export module audio to file",
    editModuleDetailsHintText: "Edit module details",
    instrumentNameLabel: "Name:",
    instrumentDefaultVolumeLabel: "Default Volume:",
    instrumentTransposeLabel: "Transpose:",
    instrumentSampleLengthLabel: "Sample Length:",
    instrumentSampleLoopsLabel: "Sample Loops:",
    instrumentLoopStartLabel: "Loop Start:",
    instrumentLoopEndLabel: "Loop End:",
    loadSampleButtonLabel: "Load Sample",
    deleteSampleButtonLabel: "Delete Sample",
    exportSampleButtonLabel: "Export Sample",
    exportSampleSucceeded: "Sample exported successfully.",
    insertSequencePositionBeforeCurrentHintText: "Insert new sequence position before current (shift-click to create new pattern)",
    insertSequencePositionAfterCurrentHintText: "Insert new sequence position after current (shift-click to create new pattern)",
    deleteSequencePositionHintText: "Delete sequence position at current",
    incrementPatternHintText: "Increment pattern at this sequence position",
    decrementPatternHintText: "Decrement pattern at this sequence position",
    startPlaybackHintText: "Start playback",
    pausePlaybackHintText: "Pause playback",
    togglePlayHintText: "Toggle play/pause",
    enableLoopModeHintText: "Enable pattern loop mode",
    disableLoopModeHintText: "Disable pattern loop mode",
    toggleLoopModeHintText: "Toggle pattern loop mode",
    seekSequenceForwardsHintText: "Seek sequence forwards",
    seekSequenceBackwardsHintText: "Seek sequence backwards",
    editTempoHintText: "Edit Tempo",
    setTempoButtonLabel: "Set tempo",
    saveButtonLabel: "Save",
    cancelButtonLabel: "Cancel",
    outputDeviceLabel: "Audio Output Device:",
    midiInputDeviceLabel: "MIDI Input Device:",
    defaultOutputDevice: "Default",
    noMidiInputDevice: "None",
    appSettingsFailed: "Could not apply settings",
    defaultAuthorNameLabel: "Default Author Name:",
    defaultTrackCountLabel: "Default Track Count:",
    defaultLinesPerBeatLabel: "Default Lines per Beat:",
    defaultBeatsPerMinuteLabel: "Default Tempo (bpm):",
    yes: "Yes",
    no: "No",
    shiftKeyboardOctaveDown: "Shift keyboard 1 octave down",
    shiftKeyboardOctaveUp: "Shift keyboard 1 octave up",
    moduleCreatedSuccessfully: "Module created successfully.",
    audioExportComplete: "Audio export completed successfully.",
    welcomeMessage: "Welcome to Arctracker!",
    hexCalculatorButtonTooltip: "Open hex calculator",
    decimalValueLabel: "Decimal:",
    hexValueLabel: "Hex:",
  },
} as const;

const messageFns = {
  en: {
    instrumentTitle: (p: string) => `Instrument ${p}`,
    jumpToSequencePositionHintText: (p: string) => `Jump to position ${p}`,
    moduleLoadedSuccessfully: (p: string) => `Modfile ${p} loaded successfully.`,
    moduleSavedSuccessfully: (p: string) => `Modfile ${p} saved successfully.`,
    audioSubsystemFailure: (p: string) => `Audio subsystem failure. Error details: ${p}`,
    errorSavingConfig: (p: string) => `Error saving config: ${p}`,
    failedToLoadConfig: (p: string) => `Failed to load config. Error details: ${p}`,
    audioSystemError: (p: string) => `Audio subsystem encountered error and will attempt restart. Error details: ${p}`,
    menuListenersFailed: (p: string) => `Failed to install menu listeners. Error details: ${p}`,
    copiedEvents: (p: string) => p === "1" ? "Copied 1 event to clipboard." : `Copied ${p} events to clipboard.`,
    copiedPattern: (p: string) => `Copied pattern ${p} to clipboard.`,
  },
} as const;

const messageFns2 = {
  en: {
    menuActionFailed: (eventId: string, errorMessage: string) => `Menu action failed: ${eventId}. Error details: ${errorMessage}`,
    copiedTrack: (trackIndex: string, patternNo: string) => `Copied pattern ${patternNo} track ${trackIndex} to clipboard.`,
  },
} as const;

let currentLanguage: keyof typeof messages = "en";

export function message(key: keyof typeof messages.en): string {
  return messages[currentLanguage][key];
}

export function messageFn(
  key: keyof typeof messageFns.en,
): (p: string) => string {
  return messageFns[currentLanguage][key];
}

export function messageFn2(
  key: keyof typeof messageFns2.en,
): (p1: string, p2: string) => string {
  return messageFns2[currentLanguage][key];
}
