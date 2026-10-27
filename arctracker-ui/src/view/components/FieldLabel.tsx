import './FieldLabel.css'

export default function FieldLabel({
  text,
  className,
  associatedField,
  hotkey,
}: {
  text: string;
  className: string;
  associatedField?: string;
  hotkey?: number;
}) {
  return (
    <div className={`${className} padded fieldLabel`}>
      {hotkey && <div className="hotKey">{hotkey}</div>}
      <label htmlFor={associatedField}>{text}</label>
    </div>
  );
}
