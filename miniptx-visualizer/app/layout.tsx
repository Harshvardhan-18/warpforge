import type { Metadata } from 'next';
import { Inter, JetBrains_Mono } from 'next/font/google';
import './globals.css';

const inter = Inter({
  subsets: ['latin'],
  variable: '--font-sans',
});

const jetbrains = JetBrains_Mono({
  subsets: ['latin'],
  variable: '--font-mono',
});

export const metadata: Metadata = {
  title: 'WarpForge — GPU Compiler Pipeline Explorer',
  description:
    'Interactive visualization of the WarpForge compiler pipeline: lexer, parser, IR, register allocation, and PTX code generation.',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html
      lang="en"
      className={`dark ${inter.variable} ${jetbrains.variable}`}
      style={{ background: '#0a0a0f' }}
    >
      <body className="min-h-screen bg-bg text-foreground antialiased">
        {children}
      </body>
    </html>
  );
}
