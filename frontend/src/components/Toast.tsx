import { useEffect } from "react";
import type { Notice } from "../ws/reducer";

export type ToastProps = {
  notice: Notice | null;
  onDismiss: () => void;
  autoDismissMs?: number;
};

function classFor(notice: Notice): string {
  switch (notice.kind) {
    case "reconnecting":
      return "bg-amber-100 text-amber-900 border-amber-300";
    case "reconnected":
      return "bg-emerald-100 text-emerald-900 border-emerald-300";
    case "error":
      return "bg-rose-100 text-rose-900 border-rose-300";
    case "warning":
      return "bg-amber-100 text-amber-900 border-amber-300";
  }
}

function messageFor(notice: Notice): string {
  switch (notice.kind) {
    case "reconnecting":
      return "Reconnecting…";
    case "reconnected":
      return "Reconnected.";
    case "error":
      return notice.message;
    case "warning":
      return notice.message;
  }
}

export default function Toast({
  notice,
  onDismiss,
  autoDismissMs = 3000,
}: ToastProps) {
  useEffect(() => {
    if (notice === null) {
      return;
    }
    if (notice.kind === "reconnecting") {
      return;
    }
    const timer = setTimeout(onDismiss, autoDismissMs);
    return () => clearTimeout(timer);
  }, [notice, onDismiss, autoDismissMs]);

  if (notice === null) {
    return null;
  }

  return (
    <div
      className={`fixed top-4 right-4 z-50 border rounded-md px-3 py-2 text-sm shadow-md ${classFor(notice)}`}
      role="status"
      aria-live="polite"
    >
      {messageFor(notice)}
    </div>
  );
}
