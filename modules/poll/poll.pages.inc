<?php
// $Id: poll.pages.inc,v 1.16 2009-04-26 19:44:39 dries Exp $

/**
 * @file
 * User page callbacks for the poll module.
 */

/**
 * Menu callback to provide a simple list of all polls available.
 */
function poll_page() {
  $polls_per_page = 15;

  $count_select = db_select('node', 'n');
  $count_select->addExpression('COUNT(*)', 'expression');
  $count_select->join('poll', 'p', 'p.nid = n.nid');
  $count_select->condition('n.status', 1);

  // List all polls.
  $select = db_select('node', 'n');
  $select->join('poll', 'p', 'p.nid = n.nid');
  $select->join('poll_choice', 'c', 'c.nid = n.nid');
  $select->addExpression('SUM(c.chvotes)', 'votes');
  $select = $select->fields('n', array('nid', 'title', 'created'))
    ->fields('p', array('active'))
    ->condition('n.status', 1)
    ->orderBy('n.created', 'DESC')
    ->groupBy('n.nid')
    ->groupBy('n.title')
    ->groupBy('p.active')
    ->groupBy('n.created')
    ->extend('PagerDefault')
    ->limit($polls_per_page)
    ->addTag('node_access');
  $select->setCountQuery($count_select);
  $queried_nodes = $select->execute()
    ->fetchAllAssoc('nid');

  $output = '<ul>';
  foreach ($queried_nodes as $node) {
    $output .= '<li>' . l($node->title, "node/$node->nid") . ' - ' . format_plural($node->votes, '1 vote', '@count votes') . ' - ' . ($node->active ? t('open') : t('closed')) . '</li>';
  }
  $output .= '</ul>';
  $output .= theme("pager", NULL, $polls_per_page);
  return $output;
}

/**
 * Callback for the 'votes' tab for polls you can see other votes on
 */
function poll_votes($node) {
  $votes_per_page = 20;
  drupal_set_title($node->title);
  $output = t('This table lists all the recorded votes for this poll. If anonymous users are allowed to vote, they will be identified by the IP address of the computer they used when they voted.');

  $header[] = array('data' => t('Visitor'), 'field' => 'u.name');
  $header[] = array('data' => t('Vote'), 'field' => 'pc.chtext');
  $header[] = array('data' => t('Timestamp'), 'field' => 'pv.timestamp', 'sort' => 'desc');

  $select = db_select('poll_vote', 'pv');
  $select->join('poll_choice', 'pc', 'pv.chid = pc.chid');
  $select->join('users', 'u', 'pv.uid = u.uid');
  $queried_votes = $select->fields('pv', array('chid', 'uid', 'hostname', 'timestamp', 'nid'))
    ->fields('pc', array('chtext'))
    ->fields('u', array('name'))
    ->condition('pv.nid', $node->nid)
    ->extend('PagerDefault')
    ->limit($votes_per_page)
    ->extend('TableSort')
    ->setHeader($header)
    ->execute()
    ->fetchAllAssoc('hostname');

  $rows = array();
  foreach ($queried_votes as $vote) {
    $rows[] = array(
      $vote->name ? theme('username', $vote) : check_plain($vote->hostname),
      check_plain($vote->chtext),
      format_date($vote->timestamp),
    );
  }
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL);
  return $output;
}

/**
 * Callback for the 'results' tab for polls you can vote on
 */
function poll_results($node) {
  drupal_set_title($node->title);
  $node->show_results = TRUE;
  return node_show($node);
}
