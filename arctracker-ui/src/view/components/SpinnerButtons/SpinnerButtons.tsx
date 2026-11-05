import "./SpinnerButtons.css";
import React from "react";

type SpinnerButtonProps = {
  onClick: () => void;
};

export type SpinnerButtonsProps = {
  incrementFn: () => void;
  decrementFn: () => void;
}

export const getSpinnerKeyboardEventHandler = (
  incrementFn: () => void,
  decrementFn: () => void,
  additionalHandler?: (e: React.KeyboardEvent) => void,
): React.KeyboardEventHandler<HTMLInputElement> => {
  return (e) => {
    if (e.key === "ArrowUp") {
      e.preventDefault();
      incrementFn();
    }
    if (e.key === "ArrowDown") {
      e.preventDefault();
      decrementFn();
    }
    if (additionalHandler) {
      additionalHandler(e);
    }
  };
};

const IncrementButton = ({ onClick }: SpinnerButtonProps) => {
  return (
    <button type="button" className={"increment"} onClick={onClick}>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        height="24px"
        viewBox="0 -960 960 960"
        width="24px"
        fill="currentColor"
      >
        <path d="m280-400 200-200 200 200H280Z" />
      </svg>
    </button>
  );
};

const DecrementButton = ({ onClick }: SpinnerButtonProps) => {
  return (
    <button type="button" className={"decrement"} onClick={onClick}>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        height="24px"
        viewBox="0 -960 960 960"
        width="24px"
        fill="currentColor"
      >
        <path d="M480-360 280-560h400L480-360Z" />
      </svg>
    </button>
  );
};

export const SpinnerButtons = ({ incrementFn, decrementFn }: SpinnerButtonsProps) => {
  return (
    <div className="spinnerButtons">
      <IncrementButton onClick={incrementFn} />
      <DecrementButton onClick={decrementFn} />
    </div>
  );
};
