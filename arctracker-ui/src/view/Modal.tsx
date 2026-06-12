import "./Modal.css";
import React from "react";

interface ModalProps {
  className?: string;
  children: React.ReactNode;
}

export default function Modal({ className, children }: ModalProps) {
  return (
    <div className="modalOverlay">
      <div className={`modalDialog ${className ?? ""}`}>
        {children}
      </div>
    </div>
  )
}
