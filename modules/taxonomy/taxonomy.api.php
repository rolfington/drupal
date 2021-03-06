<?php
// $Id: taxonomy.api.php,v 1.7 2009-03-30 05:18:49 webchick Exp $

/**
 * @file
 * Hooks provided by the Taxonomy module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Act on taxonomy vocabularies when loaded.
 *
 * Modules implementing this hook can act on the vocabulary object returned by
 * taxonomy_vocabulary_load().
 *
 * @param $vocabulary
 *   A taxonomy vocabulary object.
 */
function hook_taxonomy_vocabulary_load($vocabularies) {
  foreach ($vocabularies as $vocabulary) {
    $vocabulary->synonyms = variable_get('taxonomy_' . $vocabulary->vid . '_synonyms', FALSE);
  }
}

/**
 * Act on taxonomy vocabularies when inserted.
 *
 * Modules implementing this hook can act on the vocabulary object when saved
 *  to the database.
 *
 * @param $vocabulary
 *   A taxonomy vocabulary object.
 */
function hook_taxonomy_vocabulary_insert($vocabulary) {
  if ($vocabulary->synonyms) {
    variable_set('taxonomy_' . $vocabulary->vid . '_synonyms', TRUE);
  }
}

/**
 * Act on taxonomy vocabularies when updated.
 *
 * Modules implementing this hook can act on the term object when updated.
 *
 * @param $term
 *   A taxonomy term object, passed by reference.
 */
function hook_taxonomy_vocabulary_update($term) {
  $status = $vocabulary->synonyms ? TRUE : FALSE;
  if ($vocabulary->synonyms) {
    variable_set('taxonomy_' . $vocabulary->vid . '_synonyms', $status);
  }
}

/**
 * Respond to the deletion of taxonomy vocabularies.
 *
 * Modules implementing this hook can respond to the deletion of taxonomy
 * vocabularies from the database.
 *
 * @param $vocabulary
 *   A taxonomy vocabulary object.
 */
function hook_taxonomy_vocabulary_delete($vocabulary) {
  if (variable_get('taxonomy_' . $vocabulary->vid . '_synonyms', FALSE)) {
    variable_del('taxonomy_' . $vocabulary->vid . '_synonyms');
  }
}

/**
 * Act on taxonomy terms when loaded.
 *
 * Modules implementing this hook can act on the term object returned by
 * taxonomy_term_load().
 * For performance reasons, information to be added to term objects should be
 * loaded in a single query for all terms where possible.
 *
 * Since terms are stored and retrieved from cache during a page request, avoid
 * altering properties provided by the {taxonomy_term_data} table, since this may
 * affect the way results are loaded from cache in subsequent calls.
 *
 * @param $terms
 *   An array of term objects, indexed by tid.
 */
function hook_taxonomy_term_load($terms) {
  $result = db_query('SELECT tid, foo FROM {mytable} WHERE tid IN (:tids)', array(':tids' => array_keys($terms)));
  foreach ($result as $record) {
    $terms[$record->tid]->foo = $record->foo;
  }
}

/**
 * Act on taxonomy terms when inserted.
 *
 * Modules implementing this hook can act on the term object when saved to
 * the database.
 *
 * @param $term
 *   A taxonomy term object.
 */
function hook_taxonomy_term_insert($term) {
  if (!empty($term->synonyms)) {
    foreach (explode ("\n", str_replace("\r", '', $term->synonyms)) as $synonym) {
      if ($synonym) {
        db_insert('taxonomy_term_synonym')
        ->fields(array(
          'tid' => $term->tid,
          'name' => rtrim($synonym),
        ))
        ->execute();
      }
    }
  }
}

/**
 * Act on taxonomy terms when updated.
 *
 * Modules implementing this hook can act on the term object when updated.
 *
 * @param $term
 *   A taxonomy term object.
 */
function hook_taxonomy_term_update($term) {
  hook_taxonomy_term_delete($term);
  if (!empty($term->synonyms)) {
    foreach (explode ("\n", str_replace("\r", '', $term->synonyms)) as $synonym) {
      if ($synonym) {
        db_insert('taxonomy_term_synonym')
        ->fields(array(
          'tid' => $term->tid,
          'name' => rtrim($synonym),
        ))
        ->execute();
      }
    }
  }
}

/**
 * Respond to the deletion of taxonomy terms.
 *
 * Modules implementing this hook can respond to the deletion of taxonomy
 * terms from the database.
 *
 * @param $term
 *   A taxonomy term object.
 */
function hook_taxonomy_term_delete($term) {
  db_delete('term_synoynm')->condition('tid', $term->tid)->execute();
}

/**
 * @} End of "addtogroup hooks".
 */
