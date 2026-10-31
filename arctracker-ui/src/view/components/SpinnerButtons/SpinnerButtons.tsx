import './SpinnerButtons.css';
import React from "react";

export type SpinnerButtonProps = {
  onClick: React.MouseEventHandler<HTMLButtonElement>;
};

export const IncrementButton = ({ onClick }: SpinnerButtonProps) => {
  return (
    <button
      type="button"
      className={"increment"}
      onClick={onClick}
    >
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

export const DecrementButton = ({ onClick }: SpinnerButtonProps) => {
  return (
    <button
      type="button"
      className={"decrement"}
      onClick={onClick}
    >
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
