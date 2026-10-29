# Graph execution illustration

Open `index.html` directly in a browser. The page has no build step or external dependency.

## Editable files

- `index.html` — semantic page structure and the isometric SVG geometry.
- `graph-execution.css` — light theme, component states, connection animation, and responsive layout.
- `graph-execution.js` — five-step demonstration, branch selection, controls, and reduced-motion behavior.

## Interaction

- **Step** advances one illustrative payload through the graph.
- **Run** plays the sequence.
- **Reset** returns every component to `Idle`.
- **Branch** switches between `output1` and `output2`.
- Keyboard focus and reduced-motion preferences are supported.

## Scope note

The figure is a design explanation, not an assertion about every implementation detail. It deliberately does not prescribe queue ordering, worker count, parallel timing, mailbox capacity, or payload schema. Those assumptions are stated in the page itself.
