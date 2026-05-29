import "./Modal.css";
import React from "react";

interface ModalProps {
  children: React.ReactNode;
}

export default function Modal({ children }: ModalProps) {
  return (
    <div className="modalOverlay">
      <div className="modalDialog">
        {children}
      </div>
    </div>
  )
}
