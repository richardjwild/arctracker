import "./HexCalculator.css";
import { message } from "../language/messages.ts";
import { useStore } from "../store/useStore.ts";
import Modal from "./Modal.tsx";
import { useEffect, useRef, useState } from "react";
import { commands } from "../control/commands.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import FieldLabel from "./components/FieldLabel.tsx";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { useKeyboardHandler } from "../hooks/useKeyboardHandler.ts";

export default function HexCalculator() {
  const calculatorActive = useStore((state) => state.hexCalculatorActive);
  const [decimalValue, setDecimalValue] = useState("");
  const [hexValue, setHexValue] = useState("");
  const decimalValueInputRef = useRef<HTMLInputElement>(null);
  const hexValueInputRef = useRef<HTMLInputElement>(null);

  const handleKeys: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      commands.closeHexCalculator();
      return true;
    } else if (e.code === 'Digit1' && primaryModifier(e)) {
      decimalValueInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit2' && primaryModifier(e)) {
      hexValueInputRef.current?.focus();
      return true;
    }
    return false;
  };

  useKeyboardHandler(handleKeys, calculatorActive);

  useEffect(() => {
    if (!calculatorActive) return;
    setDecimalValue("");
    setHexValue("");
  }, [calculatorActive])

  const calculateHexValue = () => {
    const decimal = Number(decimalValue);
    if (!Number.isInteger(decimal) || decimal < 0 || decimal > 255) {
      setHexValue("!!");
      return;
    }
    setHexValue(hexadecimal.toHex(decimal, 2));
  };

  const calculateDecimalValue = () => {
    const hex = hexadecimal.fromHex(hexValue);
    if (hex === null) {
      setDecimalValue("!!!");
    } else {
      setDecimalValue(hex.toString());
      setHexValue(hexValue.toUpperCase());
    }
  };

  const HexCalculator = (
    <Modal className="hexCalculator">
      <FieldLabel
        className="decimalValueLabel"
        associatedField="decimalValueInput"
        text={message("decimalValueLabel")}
        hotkey={1}
      />
      <div className="decimalValueEdit uiArea padded rounded">
        <input
          type="text"
          id="decimalValueInput"
          maxLength={3}
          value={decimalValue}
          onBlur={calculateHexValue}
          onChange={(e) => setDecimalValue(e.target.value)}
          ref={decimalValueInputRef}
        />
      </div>
      <FieldLabel
        className="hexValueLabel"
        associatedField="hexValueInput"
        text={message("hexValueLabel")}
        hotkey={2}
      />
      <div className="hexValueEdit uiArea padded rounded">
        <input
          type="text"
          id="hexValueInput"
          maxLength={2}
          value={hexValue}
          onBlur={calculateDecimalValue}
          onChange={(e) => setHexValue(e.target.value)}
          ref={hexValueInputRef}
        />
      </div>
    </Modal>
  );

  return (
    <>
      {calculatorActive && HexCalculator}
      <div className="hexCalculatorArea">
        <button
          type="button"
          className="left right"
          title={message("hexCalculatorButtonTooltip")}
          onClick={commands.openHexCalculator}
        >
          Hex
        </button>
      </div>
    </>
  );
}
