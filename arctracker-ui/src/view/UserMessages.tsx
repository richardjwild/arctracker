import "./UserMessages.css";

const welcomeIcon = "👋";
const infoIcon = "ℹ️";
// const warningIcon = "⚠️";

export default function UserMessages() {
  return (
    <div className="userMessages">
      <div className="messages">
        <div className="message">{welcomeIcon} Welcome to Arctracker!</div>
        <div className="message">{infoIcon} User messages will be displayed here.</div>
      </div>
    </div>
  );
}
