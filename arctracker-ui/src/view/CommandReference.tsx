import { message } from "../language/messages";
import "./CommandReference.css";
import Modal from "./Modal";
import { useStore } from "../store/useStore.ts";

const commandsColumn1 = [
  [" 1", message("command01")],
  [" 2", message("command02")],
  [" 3", message("command03")],
  [" 4", message("command04")],
  [" 5", message("command05")],
  [" 6", message("command06")],
  [" 7", message("command07")],
  [" 8", message("command08")],
  [" 9", message("command09")],
  [" A", message("command0A")],
  [" B", message("command0B")],
  [" C", message("command0C")],
  [" D", message("command0D")],
  [" F", message("command0F")],
  ["12", message("command12")],
  ["18", message("command18")],
];

const commandsColumn2 = [
  ["1E", message("command1E")],
  ["E0", message("commandE0")],
  ["E1", message("commandE1")],
  ["E2", message("commandE2")],
  ["E3", message("commandE3")],
  ["E4", message("commandE4")],
  ["E6", message("commandE6")],
  ["E7", message("commandE7")],
  ["E8", message("commandE8")],
  ["E9", message("commandE9")],
  ["EA", message("commandEA")],
  ["EB", message("commandEB")],
  ["EC", message("commandEC")],
  ["ED", message("commandED")],
  ["EE", message("commandEE")],
];

export default function CommandReference() {
  const visible = useStore((state) => state.commandReferenceVisible);
  if (!visible) return null;
  return (
    <Modal className="commandReference">
      <h1>{message("commandReferenceTitle")}</h1>
      <div className="commandList uiArea padded rounded">
        {[commandsColumn1, commandsColumn2].map((commands, column) => (
          <div key={column} className="commandListColumn">
            {commands.map(([command, description]) => (
              <p key={command}>
                <span className="command"><pre>{command}</pre></span>
                <span className="description">{description}</span>
              </p>
            ))}
          </div>
        ))}
      </div>
    </Modal>
  );
}
