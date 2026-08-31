export const hexadecimal = {
  fromHexDigit: (digitStr: string): number | null => {
    if (!/^[0-9A-F]$/i.test(digitStr)) return null;
    return parseInt(digitStr, 16);
  },

  fromHex: (s: string): number | null => {
    const n = parseInt(s, 16);
    if (isNaN(n)) return null;
    return n;
  },

  toHex: (n: number, digits: number = 1): string => {
    return n.toString(16).padStart(digits, "0").toUpperCase();
  },
}
