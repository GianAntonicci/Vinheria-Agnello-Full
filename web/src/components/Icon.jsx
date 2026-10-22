const paths = {
  home: 'm3 10 9-7 9 7v10a1 1 0 0 1-1 1h-5v-7H9v7H4a1 1 0 0 1-1-1Z',
  devices: 'M4 3h16v12H4z M8 21h8 M12 15v6 M7 7h3 M7 11h6',
  settings:
    'M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8 M9 3h6l1 3 3 1 2 5-2 5-3 1-1 3H9l-1-3-3-1-2-5 2-5 3-1Z',
  logout: 'M9 4H4v16h5 M13 7l5 5-5 5 M8 12h13',
  wine: 'M8 3h8l1 7a5 5 0 0 1-10 0Z M12 15v6 M8 21h8 M7 9h10',
  thermometer: 'M9 14V5a3 3 0 0 1 6 0v9a5 5 0 1 1-6 0 M12 8v10',
  droplet: 'M12 3S5 10 5 14a7 7 0 0 0 14 0c0-4-7-11-7-11Z M8 14a4 4 0 0 0 4 4',
  sun: 'M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8 M12 2v2 M12 20v2 M2 12h2 M20 12h2 M5 5l1 1 M18 18l1 1 M5 19l1-1 M18 6l1-1',
  shield: 'm12 3 8 3v6c0 5-8 9-8 9s-8-4-8-9V6Z m-4 9 3 3 5-6',
  up: 'M5 16 16 5 M5 5h11v11',
  down: 'M5 8 16 19 M5 19h11V8',
  arrow: 'M5 12h14 m-5-5 5 5-5 5',
  plus: 'M12 5v14 M5 12h14',
  close: 'm6 6 12 12 M6 18 18 6',
  menu: 'M4 6h16 M4 12h16 M4 18h16',
  refresh: 'M20 7v5h-5 M4 17v-5h5 M6 6a8 8 0 0 1 14 6 M18 18a8 8 0 0 1-14-6',
  edit: 'm15 4 5 5 M4 20l5-1L21 7l-5-5L4 14Z',
  trash: 'M3 6h18 M9 6V3h6v3 M5 6l1 15h12l1-15 M10 10v7 M14 10v7',
  eye: 'M2 12s4-7 10-7 10 7 10 7-4 7-10 7S2 12 2 12Z M12 9a3 3 0 1 0 0 6 3 3 0 0 0 0-6',
  moon: 'M20 15A9 9 0 0 1 9 4a9 9 0 1 0 11 11Z',
  server: 'M3 3h18v7H3z M3 14h18v7H3z M7 6v1 M7 17v1 M11 6h6 M11 17h6',
  check: 'm5 12 4 4L19 6',
  alert: 'm12 3 10 18H2Z M12 9v5 M12 17v1',
  light: 'M9 18h6 M10 21h4 M9 15a6 6 0 1 1 6 0v3H9Z',
  clock: 'M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18 M12 7v5l3 2',
}
export default function Icon({ name, size = 20, ...props }) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
      {...props}
    >
      <path d={paths[name] || paths.shield} />
    </svg>
  )
}
