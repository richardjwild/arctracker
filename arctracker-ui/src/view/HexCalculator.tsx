import "./HexCalculator.css";
import { message } from "../language/messages.ts";
import { useStore } from "../store/useStore.ts";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { useState } from "react";
import { commands } from "../control/commands.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";

export default function HexCalculator() {
  const calculatorActive = useStore((state) => state.hexCalculatorActive);
  const [decimalValue, setDecimalValue] = useState("");
  const [hexValue, setHexValue] = useState("");

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
      return;
    }
    setDecimalValue(hex.toString());
  };

  const HexCalculator = (
    <Modal className="hexCalculator">
      <div className="decimalValueLabel">
        <label htmlFor="decimalValueInput">{message("decimalValueLabel")}</label>
      </div>
      <div className="decimalValueEdit uiArea padded rounded">
        <input
          type="text"
          id="decimalValueInput"
          maxLength={3}
          value={decimalValue}
          onFocus={editor.startTextInput}
          onBlur={() => {
            calculateHexValue();
            editor.stopTextInput();
          }}
          onChange={(e) => setDecimalValue(e.target.value)}
        />
      </div>
      <div className="hexValueLabel">
        <label htmlFor="hexValueInput">{message("hexValueLabel")}</label>
      </div>
      <div className="hexValueEdit uiArea padded rounded">
        <input
          type="text"
          id="hexValueInput"
          maxLength={2}
          value={hexValue}
          onFocus={editor.startTextInput}
          onBlur={() => {
            calculateDecimalValue();
            editor.stopTextInput();
          }}
          onChange={(e) => setHexValue(e.target.value)}
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
          className="hexCalculatorButton"
          title={message("hexCalculatorButtonTooltip")}
          onClick={commands.openHexCalculator}
        >
          Hex
        </button>
      </div>
    </>
  );
}
