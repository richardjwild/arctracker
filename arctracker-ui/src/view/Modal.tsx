import "./Modal.css";
import React, { RefObject } from "react";

interface ModalProps {
  className?: string;
  ref?:  RefObject<HTMLDivElement | null>
  children: React.ReactNode;
}

export default function Modal({ className, ref, children }: ModalProps) {
  return (
    <div ref={ref} tabIndex={-1} className="modalOverlay">
      <div className={`modalDialog ${className ?? ""}`}>
        {children}
      </div>
    </div>
  )
}
