import "./UserMessages.css";
import { useStore } from "../store/useStore.ts";
import { UserMessage } from "../messages/userMessages.ts";

interface MessageProps {
  message?: UserMessage;
}

function Message({ message }: MessageProps) {
  return (
    <div className="message">
      {message && (
        <>
          {getIcon(message)} {message.message}
        </>
      )}
    </div>
  );
}

function getIcon(message: UserMessage): string {
  switch (message.type) {
    case "welcome":
      return "👋";
    case "info":
      return "ℹ️";
    case "warning":
      return "⚠️";
  }
}

export default function UserMessages() {
  const messages = useStore((state) => state.userMessages);
  const recentMessages = messages.slice(-2);
  const secondLatest = recentMessages.length === 2 ? recentMessages[0] : undefined;
  const latest = recentMessages[recentMessages.length - 1];

  return (
    <div className="userMessages">
      <div className="messages">
        <Message message={secondLatest} />
        <Message message={latest} />
      </div>
    </div>
  );
}
